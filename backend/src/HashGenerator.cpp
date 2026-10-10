#include "HashGenerator.hpp"

#include <openssl/evp.h>

#include <fstream>
#include <iomanip>
#include <sstream>
#include <stdexcept>

std::string HashGenerator::generateHash(
    const std::filesystem::path& file
) const
{
    std::ifstream input(file, std::ios::binary);

    if (!input)
    {
        throw std::runtime_error(
            "Unable to open file: " + file.string()
        );
    }

    EVP_MD_CTX* context = EVP_MD_CTX_new();

    if (context == nullptr)
    {
        throw std::runtime_error("Failed to create OpenSSL context.");
    }

    if (EVP_DigestInit_ex(context, EVP_sha256(), nullptr) != 1)
    {
        EVP_MD_CTX_free(context);
        throw std::runtime_error("Failed to initialize SHA-256.");
    }

    constexpr std::size_t BUFFER_SIZE = 8192;
    char buffer[BUFFER_SIZE];

    while (input.good())
    {
        input.read(buffer, BUFFER_SIZE);
        std::streamsize bytesRead = input.gcount();

        if (bytesRead > 0)
        {
            EVP_DigestUpdate(
                context,
                buffer,
                static_cast<std::size_t>(bytesRead)
            );
        }
    }

    unsigned char hash[EVP_MAX_MD_SIZE];
    unsigned int hashLength = 0;

    if (EVP_DigestFinal_ex(context, hash, &hashLength) != 1)
    {
        EVP_MD_CTX_free(context);
        throw std::runtime_error("Failed to finalize SHA-256.");
    }

    EVP_MD_CTX_free(context);

    std::ostringstream output;

    output << std::hex << std::setfill('0');

    for (unsigned int i = 0; i < hashLength; ++i)
    {
        output << std::setw(2)
               << static_cast<int>(hash[i]);
    }

    return output.str();
}