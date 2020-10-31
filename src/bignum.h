// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2012 The Bitcoin developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.
#ifndef BITCOIN_BIGNUM_H
#define BITCOIN_BIGNUM_H

// Check windows
#if _WIN32 || _WIN64
#if _WIN64
#define SIXTY_FOUR_BIT
#else
#define THIRTY_TWO_BIT
#endif
#endif

// Check GCC
#if __GNUC__
#if __x86_64__ || __ppc64__
#define SIXTY_FOUR_BIT
#else
#define THIRTY_TWO_BIT
#endif
#endif

#include <stdexcept>
#include <vector>
//#include <bignum/bn_misc.h>
#include <bignum/bn.h>

#include "util.h" // for uint64

/** Errors thrown by the bignum class */
class bignum_error : public std::runtime_error
{
public:
    explicit bignum_error(const std::string& str) : std::runtime_error(str) {}
};


/** RAII encapsulated BN_LCP_CTX (OpenSSL bignum context) */
class CAutoBN_CTX
{
protected:
    BN_LCP_CTX* pctx;
    BN_LCP_CTX* operator=(BN_LCP_CTX* pnew) { return pctx = pnew; }

public:
    CAutoBN_CTX()
    {
        pctx = BN_LCP_CTX_new();
        if (pctx == NULL)
            throw bignum_error("CAutoBN_CTX : BN_LCP_CTX_new() returned NULL");
    }

    ~CAutoBN_CTX()
    {
        if (pctx != NULL)
            BN_LCP_CTX_free(pctx);
    }

    operator BN_LCP_CTX*() { return pctx; }
    BN_LCP_CTX& operator*() { return *pctx; }
    BN_LCP_CTX** operator&() { return &pctx; }
    bool operator!() { return (pctx == NULL); }
};


/** C++ wrapper for BIGNUM (OpenSSL bignum) */
class CBigNum : public BIGNUM_LCP
{
public:
    CBigNum()
    {
        BN_LCP_init(this);
    }

    CBigNum(const CBigNum& b)
    {
        BN_LCP_init(this);
        if (!BN_LCP_copy(this, &b))
        {
            BN_LCP_clear_free(this);
            throw bignum_error("CBigNum::CBigNum(const CBigNum&) : BN_LCP_copy failed");
        }
    }

    CBigNum& operator=(const CBigNum& b)
    {
        if (!BN_LCP_copy(this, &b))
            throw bignum_error("CBigNum::operator= : BN_LCP_copy failed");
        return (*this);
    }

    ~CBigNum()
    {
        BN_LCP_clear_free(this);
    }

    //CBigNum(char n) is not portable.  Use 'signed char' or 'unsigned char'.
    CBigNum(signed char n)      { BN_LCP_init(this); if (n >= 0) setulong(n); else setint64(n); }
    CBigNum(short n)            { BN_LCP_init(this); if (n >= 0) setulong(n); else setint64(n); }
    CBigNum(int n)              { BN_LCP_init(this); if (n >= 0) setulong(n); else setint64(n); }
    CBigNum(long n)             { BN_LCP_init(this); if (n >= 0) setulong(n); else setint64(n); }
    CBigNum(int64 n)            { BN_LCP_init(this); setint64(n); }
    CBigNum(unsigned char n)    { BN_LCP_init(this); setulong(n); }
    CBigNum(unsigned short n)   { BN_LCP_init(this); setulong(n); }
    CBigNum(unsigned int n)     { BN_LCP_init(this); setulong(n); }
    CBigNum(unsigned long n)    { BN_LCP_init(this); setulong(n); }
    CBigNum(uint64 n)           { BN_LCP_init(this); setuint64(n); }
    explicit CBigNum(uint256 n) { BN_LCP_init(this); setuint256(n); }

    explicit CBigNum(const std::vector<unsigned char>& vch)
    {
        BN_LCP_init(this);
        setvch(vch);
    }

    void setulong(unsigned long n)
    {
        if (!BN_LCP_set_word(this, n))
            throw bignum_error("CBigNum conversion from unsigned long : BN_LCP_set_word failed");
    }

    unsigned long getulong() const
    {
        return BN_LCP_get_word(this);
    }

    unsigned int getuint() const
    {
        return BN_LCP_get_word(this);
    }

    int getint() const
    {
        unsigned long n = BN_LCP_get_word(this);
        if (!BN_LCP_is_negative(this))
            return (n > (unsigned long)std::numeric_limits<int>::max() ? std::numeric_limits<int>::max() : n);
        else
            return (n > (unsigned long)std::numeric_limits<int>::max() ? std::numeric_limits<int>::min() : -(int)n);
    }

    void setint64(int64 sn)
    {
        unsigned char pch[sizeof(sn) + 6];
        unsigned char* p = pch + 4;
        bool fNegative;
        uint64 n;

        if (sn < (int64)0)
        {
            // Since the minimum signed integer cannot be represented as positive so long as its type is signed, and it's not well-defined what happens if you make it unsigned before negating it, we instead increment the negative integer by 1, convert it, then increment the (now positive) unsigned integer by 1 to compensate
            n = -(sn + 1);
            ++n;
            fNegative = true;
        } else {
            n = sn;
            fNegative = false;
        }

        bool fLeadingZeroes = true;
        for (int i = 0; i < 8; i++)
        {
            unsigned char c = (n >> 56) & 0xff;
            n <<= 8;
            if (fLeadingZeroes)
            {
                if (c == 0)
                    continue;
                if (c & 0x80)
                    *p++ = (fNegative ? 0x80 : 0);
                else if (fNegative)
                    c |= 0x80;
                fLeadingZeroes = false;
            }
            *p++ = c;
        }
        unsigned int nSize = p - (pch + 4);
        pch[0] = (nSize >> 24) & 0xff;
        pch[1] = (nSize >> 16) & 0xff;
        pch[2] = (nSize >> 8) & 0xff;
        pch[3] = (nSize) & 0xff;
        BN_LCP_mpi2bn(pch, p - pch, this);
    }

    uint64 getuint64()
    {
        unsigned int nSize = BN_LCP_bn2mpi(this, NULL);
        if (nSize < 4)
            return 0;
        std::vector<unsigned char> vch(nSize);
        BN_LCP_bn2mpi(this, &vch[0]);
        if (vch.size() > 4)
            vch[4] &= 0x7f;
        uint64 n = 0;
        for (unsigned int i = 0, j = vch.size()-1; i < sizeof(n) && j >= 4; i++, j--)
            ((unsigned char*)&n)[i] = vch[j];
        return n;
    }

    void setuint64(uint64 n)
    {
        unsigned char pch[sizeof(n) + 6];
        unsigned char* p = pch + 4;
        bool fLeadingZeroes = true;
        for (int i = 0; i < 8; i++)
        {
            unsigned char c = (n >> 56) & 0xff;
            n <<= 8;
            if (fLeadingZeroes)
            {
                if (c == 0)
                    continue;
                if (c & 0x80)
                    *p++ = 0;
                fLeadingZeroes = false;
            }
            *p++ = c;
        }
        unsigned int nSize = p - (pch + 4);
        pch[0] = (nSize >> 24) & 0xff;
        pch[1] = (nSize >> 16) & 0xff;
        pch[2] = (nSize >> 8) & 0xff;
        pch[3] = (nSize) & 0xff;
        BN_LCP_mpi2bn(pch, p - pch, this);
    }

    void setuint256(uint256 n)
    {
        unsigned char pch[sizeof(n) + 6];
        unsigned char* p = pch + 4;
        bool fLeadingZeroes = true;
        unsigned char* pbegin = (unsigned char*)&n;
        unsigned char* psrc = pbegin + sizeof(n);
        while (psrc != pbegin)
        {
            unsigned char c = *(--psrc);
            if (fLeadingZeroes)
            {
                if (c == 0)
                    continue;
                if (c & 0x80)
                    *p++ = 0;
                fLeadingZeroes = false;
            }
            *p++ = c;
        }
        unsigned int nSize = p - (pch + 4);
        pch[0] = (nSize >> 24) & 0xff;
        pch[1] = (nSize >> 16) & 0xff;
        pch[2] = (nSize >> 8) & 0xff;
        pch[3] = (nSize >> 0) & 0xff;
        BN_LCP_mpi2bn(pch, p - pch, this);
    }

    uint256 getuint256()
    {
        unsigned int nSize = BN_LCP_bn2mpi(this, NULL);
        if (nSize < 4)
            return 0;
        std::vector<unsigned char> vch(nSize);
        BN_LCP_bn2mpi(this, &vch[0]);
        if (vch.size() > 4)
            vch[4] &= 0x7f;
        uint256 n = 0;
        for (unsigned int i = 0, j = vch.size()-1; i < sizeof(n) && j >= 4; i++, j--)
            ((unsigned char*)&n)[i] = vch[j];
        return n;
    }


    void setvch(const std::vector<unsigned char>& vch)
    {
        std::vector<unsigned char> vch2(vch.size() + 4);
        unsigned int nSize = vch.size();
        // BIGNUM's byte stream format expects 4 bytes of
        // big endian size data info at the front
        vch2[0] = (nSize >> 24) & 0xff;
        vch2[1] = (nSize >> 16) & 0xff;
        vch2[2] = (nSize >> 8) & 0xff;
        vch2[3] = (nSize >> 0) & 0xff;
        // swap data to big endian
        reverse_copy(vch.begin(), vch.end(), vch2.begin() + 4);
        BN_LCP_mpi2bn(&vch2[0], vch2.size(), this);
    }

    std::vector<unsigned char> getvch() const
    {
        unsigned int nSize = BN_LCP_bn2mpi(this, NULL);
        if (nSize <= 4)
            return std::vector<unsigned char>();
        std::vector<unsigned char> vch(nSize);
        BN_LCP_bn2mpi(this, &vch[0]);
        vch.erase(vch.begin(), vch.begin() + 4);
        reverse(vch.begin(), vch.end());
        return vch;
    }

    CBigNum& SetCompact(unsigned int nCompact)
    {
        unsigned int nSize = nCompact >> 24;
        std::vector<unsigned char> vch(4 + nSize);
        vch[3] = nSize;
        if (nSize >= 1) vch[4] = (nCompact >> 16) & 0xff;
        if (nSize >= 2) vch[5] = (nCompact >> 8) & 0xff;
        if (nSize >= 3) vch[6] = (nCompact >> 0) & 0xff;
        BN_LCP_mpi2bn(&vch[0], vch.size(), this);
        return *this;
    }

    unsigned int GetCompact() const
    {
        unsigned int nSize = BN_LCP_bn2mpi(this, NULL);
        std::vector<unsigned char> vch(nSize);
        nSize -= 4;
        BN_LCP_bn2mpi(this, &vch[0]);
        unsigned int nCompact = nSize << 24;
        if (nSize >= 1) nCompact |= (vch[4] << 16);
        if (nSize >= 2) nCompact |= (vch[5] << 8);
        if (nSize >= 3) nCompact |= (vch[6] << 0);
        return nCompact;
    }

    void SetHex(const std::string& str)
    {
        // skip 0x
        const char* psz = str.c_str();
        while (isspace(*psz))
            psz++;
        bool fNegative = false;
        if (*psz == '-')
        {
            fNegative = true;
            psz++;
        }
        if (psz[0] == '0' && tolower(psz[1]) == 'x')
            psz += 2;
        while (isspace(*psz))
            psz++;

        // hex string to bignum
        static const signed char phexdigit[256] = { 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, 0,1,2,3,4,5,6,7,8,9,0,0,0,0,0,0, 0,0xa,0xb,0xc,0xd,0xe,0xf,0,0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, 0,0xa,0xb,0xc,0xd,0xe,0xf,0,0,0,0,0,0,0,0,0 };
        *this = 0;
        while (isxdigit(*psz))
        {
            *this <<= 4;
            int n = phexdigit[(unsigned char)*psz++];
            *this += n;
        }
        if (fNegative)
            *this = 0 - *this;
    }

    std::string ToString(int nBase=10) const
    {
        CAutoBN_CTX pctx;
        CBigNum bnBase = nBase;
        CBigNum bn0 = 0;
        std::string str;
        CBigNum bn = *this;
        BN_LCP_set_negative(&bn, false);
        CBigNum dv;
        CBigNum rem;
        if (BN_LCP_cmp(&bn, &bn0) == 0)
            return "0";
        while (BN_LCP_cmp(&bn, &bn0) > 0)
        {
            if (!BN_LCP_div(&dv, &rem, &bn, &bnBase, pctx))
                throw bignum_error("CBigNum::ToString() : BN_LCP_div failed");
            bn = dv;
            unsigned int c = rem.getulong();
            str += "0123456789abcdef"[c];
        }
        if (BN_LCP_is_negative(this))
            str += "-";
        reverse(str.begin(), str.end());
        return str;
    }

    std::string GetHex() const
    {
        return ToString(16);
    }

    unsigned int GetSerializeSize(int nType=0, int nVersion=PROTOCOL_VERSION) const
    {
        return ::GetSerializeSize(getvch(), nType, nVersion);
    }

    template<typename Stream>
    void Serialize(Stream& s, int nType=0, int nVersion=PROTOCOL_VERSION) const
    {
        ::Serialize(s, getvch(), nType, nVersion);
    }

    template<typename Stream>
    void Unserialize(Stream& s, int nType=0, int nVersion=PROTOCOL_VERSION)
    {
        std::vector<unsigned char> vch;
        ::Unserialize(s, vch, nType, nVersion);
        setvch(vch);
    }


    bool operator!() const
    {
        return BN_LCP_is_zero(this);
    }

    CBigNum& operator+=(const CBigNum& b)
    {
        if (!BN_LCP_add(this, this, &b))
            throw bignum_error("CBigNum::operator+= : BN_LCP_add failed");
        return *this;
    }

    CBigNum& operator-=(const CBigNum& b)
    {
        *this = *this - b;
        return *this;
    }

    CBigNum& operator*=(const CBigNum& b)
    {
        CAutoBN_CTX pctx;
        if (!BN_LCP_mul(this, this, &b, pctx))
            throw bignum_error("CBigNum::operator*= : BN_LCP_mul failed");
        return *this;
    }

    CBigNum& operator/=(const CBigNum& b)
    {
        *this = *this / b;
        return *this;
    }

    CBigNum& operator%=(const CBigNum& b)
    {
        *this = *this % b;
        return *this;
    }

    CBigNum& operator<<=(unsigned int shift)
    {
        if (!BN_LCP_lshift(this, this, shift))
            throw bignum_error("CBigNum:operator<<= : BN_LCP_lshift failed");
        return *this;
    }

    CBigNum& operator>>=(unsigned int shift)
    {
        // Note: BN_LCP_rshift segfaults on 64-bit if 2^shift is greater than the number
        //   if built on ubuntu 9.04 or 9.10, probably depends on version of OpenSSL
        CBigNum a = 1;
        a <<= shift;
        if (BN_LCP_cmp(&a, this) > 0)
        {
            *this = 0;
            return *this;
        }

        if (!BN_LCP_rshift(this, this, shift))
            throw bignum_error("CBigNum:operator>>= : BN_LCP_rshift failed");
        return *this;
    }


    CBigNum& operator++()
    {
        // prefix operator
        if (!BN_LCP_add(this, this, BN_LCP_value_one()))
            throw bignum_error("CBigNum::operator++ : BN_LCP_add failed");
        return *this;
    }

    const CBigNum operator++(int)
    {
        // postfix operator
        const CBigNum ret = *this;
        ++(*this);
        return ret;
    }

    CBigNum& operator--()
    {
        // prefix operator
        CBigNum r;
        if (!BN_LCP_sub(&r, this, BN_LCP_value_one()))
            throw bignum_error("CBigNum::operator-- : BN_LCP_sub failed");
        *this = r;
        return *this;
    }

    const CBigNum operator--(int)
    {
        // postfix operator
        const CBigNum ret = *this;
        --(*this);
        return ret;
    }


    friend inline const CBigNum operator-(const CBigNum& a, const CBigNum& b);
    friend inline const CBigNum operator/(const CBigNum& a, const CBigNum& b);
    friend inline const CBigNum operator%(const CBigNum& a, const CBigNum& b);
};



inline const CBigNum operator+(const CBigNum& a, const CBigNum& b)
{
    CBigNum r;
    if (!BN_LCP_add(&r, &a, &b))
        throw bignum_error("CBigNum::operator+ : BN_LCP_add failed");
    return r;
}

inline const CBigNum operator-(const CBigNum& a, const CBigNum& b)
{
    CBigNum r;
    if (!BN_LCP_sub(&r, &a, &b))
        throw bignum_error("CBigNum::operator- : BN_LCP_sub failed");
    return r;
}

inline const CBigNum operator-(const CBigNum& a)
{
    CBigNum r(a);
    BN_LCP_set_negative(&r, !BN_LCP_is_negative(&r));
    return r;
}

inline const CBigNum operator*(const CBigNum& a, const CBigNum& b)
{
    CAutoBN_CTX pctx;
    CBigNum r;
    if (!BN_LCP_mul(&r, &a, &b, pctx))
        throw bignum_error("CBigNum::operator* : BN_LCP_mul failed");
    return r;
}

inline const CBigNum operator/(const CBigNum& a, const CBigNum& b)
{
    CAutoBN_CTX pctx;
    CBigNum r;
    if (!BN_LCP_div(&r, NULL, &a, &b, pctx))
        throw bignum_error("CBigNum::operator/ : BN_LCP_div failed");
    return r;
}

inline const CBigNum operator%(const CBigNum& a, const CBigNum& b)
{
    CAutoBN_CTX pctx;
    CBigNum r;
    if (!BN_LCP_mod(&r, &a, &b, pctx))
        throw bignum_error("CBigNum::operator% : BN_LCP_div failed");
    return r;
}

inline const CBigNum operator<<(const CBigNum& a, unsigned int shift)
{
    CBigNum r;
    if (!BN_LCP_lshift(&r, &a, shift))
        throw bignum_error("CBigNum:operator<< : BN_LCP_lshift failed");
    return r;
}

inline const CBigNum operator>>(const CBigNum& a, unsigned int shift)
{
    CBigNum r = a;
    r >>= shift;
    return r;
}

inline bool operator==(const CBigNum& a, const CBigNum& b) { return (BN_LCP_cmp(&a, &b) == 0); }
inline bool operator!=(const CBigNum& a, const CBigNum& b) { return (BN_LCP_cmp(&a, &b) != 0); }
inline bool operator<=(const CBigNum& a, const CBigNum& b) { return (BN_LCP_cmp(&a, &b) <= 0); }
inline bool operator>=(const CBigNum& a, const CBigNum& b) { return (BN_LCP_cmp(&a, &b) >= 0); }
inline bool operator<(const CBigNum& a, const CBigNum& b)  { return (BN_LCP_cmp(&a, &b) < 0); }
inline bool operator>(const CBigNum& a, const CBigNum& b)  { return (BN_LCP_cmp(&a, &b) > 0); }

#endif
