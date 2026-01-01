#pragma once

#include <cstddef>
#include <string>

namespace er2
{

inline std::string DumpSdk6ToLowerAscii(std::string s)
{
    for (char& c : s)
    {
        if (c >= 'A' && c <= 'Z')
        {
            c = static_cast<char>(c - 'A' + 'a');
        }
    }
    return s;
}

inline std::string DumpSdk6BaseNameNoExt(std::string s)
{
    const std::size_t p0 = s.find_last_of("/\\");
    if (p0 != std::string::npos)
    {
        s = s.substr(p0 + 1);
    }

    const std::string low = DumpSdk6ToLowerAscii(s);
    if (low.size() >= 4 && low.compare(low.size() - 4, 4, ".dll") == 0)
    {
        s.resize(s.size() - 4);
    }
    return s;
}

inline std::string DumpSdk6NormalizeAssemblyKey(const std::string& s)
{
    return DumpSdk6ToLowerAscii(DumpSdk6BaseNameNoExt(s));
}

inline std::string DumpSdk6JsonEscape(const std::string& s)
{
    std::string out;
    out.reserve(s.size() + 16);
    for (char c : s)
    {
        switch (c)
        {
        case '\\': out += "\\\\"; break;
        case '"': out += "\\\""; break;
        case '\n': out += "\\n"; break;
        case '\r': out += "\\r"; break;
        case '\t': out += "\\t"; break;
        default: out += c; break;
        }
    }
    return out;
}

inline bool DumpSdk6IsIdentStart(char c)
{
    return (c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z') || c == '_';
}

inline bool DumpSdk6IsIdentChar(char c)
{
    return DumpSdk6IsIdentStart(c) || (c >= '0' && c <= '9');
}

inline std::string DumpSdk6StripNamespacesInType(const std::string& s)
{
    if (s.empty())
    {
        return s;
    }

    std::string out;
    out.reserve(s.size());

    const std::size_t n = s.size();
    std::size_t i = 0;

    while (i < n)
    {
        const char c = s[i];
        if (!DumpSdk6IsIdentStart(c))
        {
            out.push_back(c);
            ++i;
            continue;
        }

        std::size_t segStart = i;
        std::size_t segEnd = i + 1;
        while (segEnd < n && DumpSdk6IsIdentChar(s[segEnd]))
        {
            ++segEnd;
        }

        std::string lastSeg = s.substr(segStart, segEnd - segStart);
        bool hasDotChain = false;

        std::size_t j = segEnd;
        while (j + 1 < n && s[j] == '.' && DumpSdk6IsIdentStart(s[j + 1]))
        {
            hasDotChain = true;
            j += 1;

            const std::size_t nextStart = j;
            std::size_t nextEnd = nextStart + 1;
            while (nextEnd < n && DumpSdk6IsIdentChar(s[nextEnd]))
            {
                ++nextEnd;
            }

            lastSeg.assign(s.substr(nextStart, nextEnd - nextStart));
            j = nextEnd;
        }

        if (hasDotChain)
        {
            out.append(lastSeg);
            i = j;
            continue;
        }

        out.append(s.substr(segStart, segEnd - segStart));
        i = segEnd;
    }

    return out;
}

inline bool DumpSdk6IsTypeReplaceBoundaryChar(char c)
{
    const bool alpha = (c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z');
    const bool digit = (c >= '0' && c <= '9');
    const bool under = (c == '_');
    const bool dot = (c == '.');
    return !(alpha || digit || under || dot);
}

inline std::string DumpSdk6ReplaceAllTypeTokens(std::string s, const std::string& from, const std::string& to)
{
    if (from.empty())
    {
        return s;
    }

    std::size_t pos = 0;
    while (pos < s.size())
    {
        pos = s.find(from, pos);
        if (pos == std::string::npos)
        {
            break;
        }

        const bool okLeft = (pos == 0) ? true : DumpSdk6IsTypeReplaceBoundaryChar(s[pos - 1]);
        const std::size_t end = pos + from.size();
        const bool okRight = (end >= s.size()) ? true : DumpSdk6IsTypeReplaceBoundaryChar(s[end]);

        if (okLeft && okRight)
        {
            s.replace(pos, from.size(), to);
            pos += to.size();
        }
        else
        {
            pos += from.size();
        }
    }

    return s;
}

inline std::string DumpSdk6ToCsType(std::string typeName)
{
    if (typeName.empty())
    {
        return typeName;
    }

    if (!typeName.empty() && typeName.back() == '&')
    {
        std::string inner = typeName.substr(0, typeName.size() - 1);
        inner = DumpSdk6ToCsType(inner);
        return inner.empty() ? typeName : ("ref " + inner);
    }

    typeName = DumpSdk6ReplaceAllTypeTokens(typeName, "System.Void", "void");
    typeName = DumpSdk6ReplaceAllTypeTokens(typeName, "System.Boolean", "bool");
    typeName = DumpSdk6ReplaceAllTypeTokens(typeName, "System.Char", "char");
    typeName = DumpSdk6ReplaceAllTypeTokens(typeName, "System.SByte", "sbyte");
    typeName = DumpSdk6ReplaceAllTypeTokens(typeName, "System.Byte", "byte");
    typeName = DumpSdk6ReplaceAllTypeTokens(typeName, "System.Int16", "short");
    typeName = DumpSdk6ReplaceAllTypeTokens(typeName, "System.UInt16", "ushort");
    typeName = DumpSdk6ReplaceAllTypeTokens(typeName, "System.Int32", "int");
    typeName = DumpSdk6ReplaceAllTypeTokens(typeName, "System.UInt32", "uint");
    typeName = DumpSdk6ReplaceAllTypeTokens(typeName, "System.Int64", "long");
    typeName = DumpSdk6ReplaceAllTypeTokens(typeName, "System.UInt64", "ulong");
    typeName = DumpSdk6ReplaceAllTypeTokens(typeName, "System.Single", "float");
    typeName = DumpSdk6ReplaceAllTypeTokens(typeName, "System.Double", "double");
    typeName = DumpSdk6ReplaceAllTypeTokens(typeName, "System.Decimal", "decimal");
    typeName = DumpSdk6ReplaceAllTypeTokens(typeName, "System.String", "string");
    typeName = DumpSdk6ReplaceAllTypeTokens(typeName, "System.Object", "object");

    return typeName;
}

} // namespace er2
