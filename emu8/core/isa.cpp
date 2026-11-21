#include "isa.h"
#include <cctype>

namespace isa {

static bool egalSansCasse(const char* a, const char* b)
{
    while (*a != '\0' && *b != '\0') {
        if (std::toupper(static_cast<unsigned char>(*a)) !=
            std::toupper(static_cast<unsigned char>(*b))) {
            return false;
        }
        ++a;
        ++b;
    }
    return *a == *b;
}

const DescInstruction* trouverParMnemonique(const char* mnemo)
{
    if (mnemo == nullptr) return nullptr;
    for (std::size_t i = 0; i < NB_INSTRUCTIONS; ++i) {
        if (egalSansCasse(TABLE[i].mnemonique, mnemo)) {
            return &TABLE[i];
        }
    }
    return nullptr;
}

const DescInstruction* trouverParOpcode(uint8_t octet)
{
    const uint8_t base = static_cast<uint8_t>(octet & MASQUE_OPCODE);
    for (std::size_t i = 0; i < NB_INSTRUCTIONS; ++i) {
        if (TABLE[i].opcode == base) {
            return &TABLE[i];
        }
    }
    return nullptr;
}

} // namespace isa
