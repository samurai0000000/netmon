/*
 * AuthManager.cxx
 *
 * Copyright (C) 2026, Charles Chiou
 */

#include "AuthManager.hxx"
#include "Config.hxx"

#include <iostream>
#include <fstream>
#include <vector>
#include <sstream>
#include <iomanip>
#include <cstring>
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>

#include <openssl/evp.h>
#include <openssl/rand.h>
#include <openssl/crypto.h>
#include <nlohmann/json.hpp>

using json = nlohmann::json;

static std::string bytesToHex(const unsigned char *data, size_t len) {
    std::ostringstream oss;
    for (size_t i = 0; i < len; ++i) {
        oss << std::hex << std::setw(2) << std::setfill('0') << static_cast<int>(data[i]);
    }
    return oss.str();
}

static std::vector<unsigned char> hexToBytes(const std::string &hex) {
    std::vector<unsigned char> bytes;
    if (hex.size() % 2 != 0) {
        return bytes;
    }
    bytes.reserve(hex.size() / 2);
    for (size_t i = 0; i < hex.size(); i += 2) {
        unsigned int byteVal = 0;
        std::stringstream ss;
        ss << std::hex << hex.substr(i, 2);
        ss >> byteVal;
        bytes.push_back(static_cast<unsigned char>(byteVal));
    }
    return bytes;
}

class ScopedLock {
public:
    ScopedLock(const std::string &lockPath, short lockType)
        : _fd(-1), _locked(false) {
        _fd = open(lockPath.c_str(), O_RDWR | O_CREAT, 0600);
        if (_fd < 0) {
            return;
        }
        fchmod(_fd, 0600);

        struct flock fl;
        std::memset(&fl, 0, sizeof(fl));
        fl.l_type = lockType;
        fl.l_whence = SEEK_SET;
        fl.l_start = 0;
        fl.l_len = 0;

        if (fcntl(_fd, F_SETLKW, &fl) == 0) {
            _locked = true;
        }
    }

    ~ScopedLock() {
        if (_fd >= 0) {
            if (_locked) {
                struct flock fl;
                std::memset(&fl, 0, sizeof(fl));
                fl.l_type = F_UNLCK;
                fl.l_whence = SEEK_SET;
                fl.l_start = 0;
                fl.l_len = 0;
                fcntl(_fd, F_SETLK, &fl);
            }
            close(_fd);
        }
    }

    bool isLocked() const {
        return _locked;
    }

private:
    int  _fd;
    bool _locked;
};

AuthManager &AuthManager::getInstance() {
    static AuthManager instance;
    return instance;
}

AuthManager::AuthManager() {
    _vaultDir = Config::resolveHomePath("~/.config/netmon");
}

void AuthManager::setVaultDir(const std::string &dir) {
    std::lock_guard<std::mutex> lock(_vaultDirMutex);
    _vaultDir = dir;
}

const std::string &AuthManager::getVaultDir() const {
    std::lock_guard<std::mutex> lock(_vaultDirMutex);
    return _vaultDir;
}

std::string AuthManager::getVaultKeyPath() const {
    std::lock_guard<std::mutex> lock(_vaultDirMutex);
    return _vaultDir + "/vault.key";
}

std::string AuthManager::getVaultEncPath() const {
    std::lock_guard<std::mutex> lock(_vaultDirMutex);
    return _vaultDir + "/vault.enc";
}

std::string AuthManager::getVaultLockPath() const {
    std::lock_guard<std::mutex> lock(_vaultDirMutex);
    return _vaultDir + "/vault.lock";
}

bool AuthManager::ensureDirectory(const std::string &dir) const {
    struct stat st;
    if (stat(dir.c_str(), &st) != 0) {
        if (mkdir(dir.c_str(), 0700) != 0) {
            return false;
        }
    }
    chmod(dir.c_str(), 0700);
    return true;
}

static bool readKey(const std::string &keyPath, std::vector<unsigned char> &keyOut) {
    std::ifstream is(keyPath, std::ios::binary);
    if (!is.is_open()) {
        return false;
    }
    keyOut.assign((std::istreambuf_iterator<char>(is)),
                  std::istreambuf_iterator<char>());
    return (keyOut.size() == 32);
}

static bool writeKey(const std::string &keyPath, const std::vector<unsigned char> &key) {
    int fd = open(keyPath.c_str(), O_WRONLY | O_CREAT | O_TRUNC, 0600);
    if (fd < 0) {
        return false;
    }
    ssize_t written = write(fd, key.data(), key.size());
    fsync(fd);
    close(fd);
    return (written == static_cast<ssize_t>(key.size()));
}

static bool encryptAesGcm(const std::vector<unsigned char> &key,
                         const std::string &plaintext,
                         std::vector<unsigned char> &cipherOut) {
    if (key.size() != 32) {
        return false;
    }

    std::vector<unsigned char> iv(12);
    if (RAND_bytes(iv.data(), 12) != 1) {
        return false;
    }

    EVP_CIPHER_CTX *ctx = EVP_CIPHER_CTX_new();
    if (!ctx) {
        return false;
    }

    if (EVP_EncryptInit_ex(ctx, EVP_aes_256_gcm(), nullptr, nullptr, nullptr) != 1 ||
        EVP_CIPHER_CTX_ctrl(ctx, EVP_CTRL_GCM_SET_IVLEN, 12, nullptr) != 1 ||
        EVP_EncryptInit_ex(ctx, nullptr, nullptr, key.data(), iv.data()) != 1) {
        EVP_CIPHER_CTX_free(ctx);
        return false;
    }

    std::vector<unsigned char> encrypted(plaintext.size() + 16);
    int outLen = 0;
    if (EVP_EncryptUpdate(ctx, encrypted.data(), &outLen,
                          reinterpret_cast<const unsigned char *>(plaintext.data()),
                          plaintext.size()) != 1) {
        EVP_CIPHER_CTX_free(ctx);
        return false;
    }

    int finalLen = 0;
    if (EVP_EncryptFinal_ex(ctx, encrypted.data() + outLen, &finalLen) != 1) {
        EVP_CIPHER_CTX_free(ctx);
        return false;
    }
    encrypted.resize(outLen + finalLen);

    std::vector<unsigned char> tag(16);
    if (EVP_CIPHER_CTX_ctrl(ctx, EVP_CTRL_GCM_GET_TAG, 16, tag.data()) != 1) {
        EVP_CIPHER_CTX_free(ctx);
        return false;
    }
    EVP_CIPHER_CTX_free(ctx);

    cipherOut.clear();
    cipherOut.reserve(12 + 16 + encrypted.size());
    cipherOut.insert(cipherOut.end(), iv.begin(), iv.end());
    cipherOut.insert(cipherOut.end(), tag.begin(), tag.end());
    cipherOut.insert(cipherOut.end(), encrypted.begin(), encrypted.end());
    return true;
}

static bool decryptAesGcm(const std::vector<unsigned char> &key,
                         const std::vector<unsigned char> &cipherIn,
                         std::string &plainOut) {
    if (key.size() != 32 || cipherIn.size() < 28) {
        return false;
    }

    const unsigned char *iv = cipherIn.data();
    const unsigned char *tag = cipherIn.data() + 12;
    const unsigned char *ciphertext = cipherIn.data() + 28;
    size_t cipherLen = cipherIn.size() - 28;

    EVP_CIPHER_CTX *ctx = EVP_CIPHER_CTX_new();
    if (!ctx) {
        return false;
    }

    if (EVP_DecryptInit_ex(ctx, EVP_aes_256_gcm(), nullptr, nullptr, nullptr) != 1 ||
        EVP_CIPHER_CTX_ctrl(ctx, EVP_CTRL_GCM_SET_IVLEN, 12, nullptr) != 1 ||
        EVP_DecryptInit_ex(ctx, nullptr, nullptr, key.data(), iv) != 1) {
        EVP_CIPHER_CTX_free(ctx);
        return false;
    }

    std::vector<unsigned char> decrypted(cipherLen + 16);
    int outLen = 0;
    if (EVP_DecryptUpdate(ctx, decrypted.data(), &outLen, ciphertext, cipherLen) != 1) {
        EVP_CIPHER_CTX_free(ctx);
        return false;
    }

    if (EVP_CIPHER_CTX_ctrl(ctx, EVP_CTRL_GCM_SET_TAG, 16, const_cast<unsigned char *>(tag)) != 1) {
        EVP_CIPHER_CTX_free(ctx);
        return false;
    }

    int finalLen = 0;
    int ret = EVP_DecryptFinal_ex(ctx, decrypted.data() + outLen, &finalLen);
    EVP_CIPHER_CTX_free(ctx);

    if (ret <= 0) {
        return false;
    }

    plainOut.assign(reinterpret_cast<char *>(decrypted.data()), outLen + finalLen);
    return true;
}

static bool readVaultFile(const std::string &encPath,
                          const std::string &keyPath,
                          json &parsedJson) {
    std::vector<unsigned char> key;
    if (!readKey(keyPath, key)) {
        return false;
    }

    std::ifstream is(encPath, std::ios::binary);
    if (!is.is_open()) {
        return false;
    }
    std::vector<unsigned char> cipherData((std::istreambuf_iterator<char>(is)),
                                         std::istreambuf_iterator<char>());
    is.close();

    std::string decryptedText;
    if (!decryptAesGcm(key, cipherData, decryptedText)) {
        return false;
    }

    try {
        parsedJson = json::parse(decryptedText);
        return true;
    } catch (...) {
        return false;
    }
}

bool AuthManager::setPassword(const std::string &newPassword,
                             const std::string &oldPassword,
                             bool requireOld) {
    if (newPassword.empty()) {
        return false;
    }

    std::string dir;
    {
        std::lock_guard<std::mutex> lock(_vaultDirMutex);
        dir = _vaultDir;
    }
    if (!ensureDirectory(dir)) {
        return false;
    }

    std::string keyPath = dir + "/vault.key";
    std::string encPath = dir + "/vault.enc";
    std::string tmpPath = dir + "/vault.enc.tmp";
    std::string lockPath = dir + "/vault.lock";

    ScopedLock lock(lockPath, F_WRLCK);
    if (!lock.isLocked()) {
        return false;
    }

    int currentGen = 0;
    json currentVault;
    bool hasExisting = readVaultFile(encPath, keyPath, currentVault);

    if (requireOld) {
        if (!hasExisting) {
            return false;
        }
        std::string storedSaltHex = currentVault.value("salt", "");
        std::string storedHashHex = currentVault.value("hash", "");
        auto saltBytes = hexToBytes(storedSaltHex);
        auto storedHashBytes = hexToBytes(storedHashHex);
        if (saltBytes.size() != 16 || storedHashBytes.size() != 32) {
            return false;
        }

        std::vector<unsigned char> testHash(32);
        if (PKCS5_PBKDF2_HMAC(oldPassword.c_str(), oldPassword.size(),
                              saltBytes.data(), saltBytes.size(),
                              210000, EVP_sha256(),
                              32, testHash.data()) != 1) {
            return false;
        }

        if (CRYPTO_memcmp(testHash.data(), storedHashBytes.data(), 32) != 0) {
            return false;
        }
    }

    if (hasExisting && currentVault.contains("generation") && currentVault["generation"].is_number_integer()) {
        currentGen = currentVault["generation"].get<int>();
    }

    int nextGen = currentGen + 1;

    std::vector<unsigned char> key;
    if (!readKey(keyPath, key)) {
        key.resize(32);
        if (RAND_bytes(key.data(), 32) != 1) {
            return false;
        }
        if (!writeKey(keyPath, key)) {
            return false;
        }
    }

    std::vector<unsigned char> salt(16);
    if (RAND_bytes(salt.data(), 16) != 1) {
        return false;
    }

    std::vector<unsigned char> hash(32);
    if (PKCS5_PBKDF2_HMAC(newPassword.c_str(), newPassword.size(),
                          salt.data(), salt.size(),
                          210000, EVP_sha256(),
                          32, hash.data()) != 1) {
        return false;
    }

    json newVault = {
        {"generation", nextGen},
        {"salt", bytesToHex(salt.data(), salt.size())},
        {"hash", bytesToHex(hash.data(), hash.size())}
    };
    std::string plainJson = newVault.dump();

    std::vector<unsigned char> cipherData;
    if (!encryptAesGcm(key, plainJson, cipherData)) {
        return false;
    }

    int tmpFd = open(tmpPath.c_str(), O_WRONLY | O_CREAT | O_TRUNC, 0600);
    if (tmpFd < 0) {
        return false;
    }
    fchmod(tmpFd, 0600);
    ssize_t written = write(tmpFd, cipherData.data(), cipherData.size());
    if (written != static_cast<ssize_t>(cipherData.size())) {
        close(tmpFd);
        unlink(tmpPath.c_str());
        return false;
    }
    fsync(tmpFd);
    close(tmpFd);

    if (rename(tmpPath.c_str(), encPath.c_str()) != 0) {
        unlink(tmpPath.c_str());
        return false;
    }

    int dirFd = open(dir.c_str(), O_RDONLY | O_DIRECTORY);
    if (dirFd >= 0) {
        fsync(dirFd);
        close(dirFd);
    }

    return true;
}

std::string AuthManager::login(const std::string &password) {
    if (password.empty()) {
        return "";
    }

    std::string dir;
    {
        std::lock_guard<std::mutex> lock(_vaultDirMutex);
        dir = _vaultDir;
    }

    std::string keyPath = dir + "/vault.key";
    std::string encPath = dir + "/vault.enc";
    std::string lockPath = dir + "/vault.lock";

    ScopedLock lock(lockPath, F_RDLCK);
    if (!lock.isLocked()) {
        return "";
    }

    json vaultJson;
    if (!readVaultFile(encPath, keyPath, vaultJson)) {
        return "";
    }

    if (!vaultJson.contains("generation") || !vaultJson.contains("salt") || !vaultJson.contains("hash")) {
        return "";
    }

    int generation = vaultJson["generation"].get<int>();
    std::string saltHex = vaultJson["salt"].get<std::string>();
    std::string hashHex = vaultJson["hash"].get<std::string>();

    auto saltBytes = hexToBytes(saltHex);
    auto hashBytes = hexToBytes(hashHex);
    if (saltBytes.size() != 16 || hashBytes.size() != 32) {
        return "";
    }

    std::vector<unsigned char> testHash(32);
    if (PKCS5_PBKDF2_HMAC(password.c_str(), password.size(),
                          saltBytes.data(), saltBytes.size(),
                          210000, EVP_sha256(),
                          32, testHash.data()) != 1) {
        return "";
    }

    if (CRYPTO_memcmp(testHash.data(), hashBytes.data(), 32) != 0) {
        return "";
    }

    // Generate 32-byte URL-safe token
    static const char charset[] =
        "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz_-";
    std::string token;
    token.resize(32);
    unsigned char randBytes[32];
    if (RAND_bytes(randBytes, sizeof(randBytes)) != 1) {
        return "";
    }
    for (size_t i = 0; i < 32; ++i) {
        token[i] = charset[randBytes[i] % 64];
    }
    OPENSSL_cleanse(randBytes, sizeof(randBytes));

    time_t now = time(nullptr);
    {
        std::lock_guard<std::mutex> sessionLock(_sessionMutex);
        Session s;
        s.token = token;
        s.issuedGeneration = generation;
        s.lastUsedTime = now;
        _sessions[token] = s;
    }

    return token;
}

bool AuthManager::validateSession(const std::string &token) {
    if (token.empty()) {
        return false;
    }

    int issuedGen = 0;
    time_t now = time(nullptr);

    {
        std::lock_guard<std::mutex> sessionLock(_sessionMutex);
        auto it = _sessions.find(token);
        if (it == _sessions.end()) {
            return false;
        }

        // 15-minute idle timeout (900 seconds)
        if (now - it->second.lastUsedTime > 900) {
            _sessions.erase(it);
            return false;
        }

        issuedGen = it->second.issuedGeneration;
    }

    int diskGen = getDiskGeneration();
    if (diskGen <= 0 || diskGen != issuedGen) {
        std::lock_guard<std::mutex> sessionLock(_sessionMutex);
        _sessions.erase(token);
        return false;
    }

    {
        std::lock_guard<std::mutex> sessionLock(_sessionMutex);
        auto it = _sessions.find(token);
        if (it != _sessions.end()) {
            it->second.lastUsedTime = now;
        }
    }

    return true;
}

void AuthManager::logout(const std::string &token) {
    if (token.empty()) {
        return;
    }
    std::lock_guard<std::mutex> sessionLock(_sessionMutex);
    _sessions.erase(token);
}

int AuthManager::getDiskGeneration() {
    std::string dir;
    {
        std::lock_guard<std::mutex> lock(_vaultDirMutex);
        dir = _vaultDir;
    }

    std::string keyPath = dir + "/vault.key";
    std::string encPath = dir + "/vault.enc";
    std::string lockPath = dir + "/vault.lock";

    ScopedLock lock(lockPath, F_RDLCK);
    if (!lock.isLocked()) {
        return 0;
    }

    json vaultJson;
    if (!readVaultFile(encPath, keyPath, vaultJson)) {
        return 0;
    }

    if (vaultJson.contains("generation") && vaultJson["generation"].is_number_integer()) {
        return vaultJson["generation"].get<int>();
    }

    return 0;
}

void AuthManager::clearSessions() {
    std::lock_guard<std::mutex> sessionLock(_sessionMutex);
    std::map<std::string, Session>().swap(_sessions);
}

void AuthManager::resetForTesting() {
    clearSessions();
    std::lock_guard<std::mutex> lock(_vaultDirMutex);
    _vaultDir.clear();
    _vaultDir.shrink_to_fit();
}

void AuthManager::setSessionTimestampForTesting(const std::string &token, time_t timestamp) {
    std::lock_guard<std::mutex> sessionLock(_sessionMutex);
    auto it = _sessions.find(token);
    if (it != _sessions.end()) {
        it->second.lastUsedTime = timestamp;
    }
}

/*
 * Local variables:
 * mode: C++
 * c-file-style: "BSD"
 * c-basic-offset: 4
 * tab-width: 4
 * indent-tabs-mode: nil
 * End:
 */
