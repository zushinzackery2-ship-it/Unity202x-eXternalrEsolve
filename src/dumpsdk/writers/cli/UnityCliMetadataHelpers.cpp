#include <er2/unity2/dumpsdk/writers/cli/UnityCliMetadataBuilder.h>

#include <algorithm>
#include <cctype>

namespace er2::UnityCli
{
	std::string ToLowerAscii(std::string value)
	{
		for (char& c : value)
			c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
		return value;
	}

	bool EndsWith(const std::string& value, std::string_view suffix)
	{
		return value.size() >= suffix.size() && std::equal(suffix.rbegin(), suffix.rend(), value.rbegin());
	}

	std::string StripExtension(std::string value)
	{
		const size_t slash = value.find_last_of("\\/");
		if (slash != std::string::npos)
			value = value.substr(slash + 1);

		const size_t dot = value.find_last_of('.');
		if (dot != std::string::npos)
			value = value.substr(0, dot);

		return value.empty() ? "UnityDummyAssembly" : value;
	}

	std::string CleanMetadataName(std::string value, const std::string& fallback)
	{
		if (value.empty())
			return fallback;

		for (char& c : value)
		{
			if (c == '\0')
				c = '_';
		}
		return value;
	}

	std::string RemoveTypePrefix(std::string value)
	{
		const std::string lower = ToLowerAscii(value);
		const std::string prefixes[] = { "class ", "struct ", "valuetype ", "enum " };
		for (const std::string& prefix : prefixes)
		{
			if (lower.starts_with(prefix))
				return value.substr(prefix.size());
		}
		return value;
	}

	std::string RemoveGenericArguments(std::string value)
	{
		const size_t angle = value.find('<');
		if (angle != std::string::npos)
			value = value.substr(0, angle);

		const size_t bracket = value.find('[');
		if (bracket != std::string::npos && !EndsWith(value, "[]"))
			value = value.substr(0, bracket);

		return value;
	}

	std::string LastNameComponent(const std::string& value)
	{
		const size_t dot = value.find_last_of('.');
		return dot == std::string::npos ? value : value.substr(dot + 1);
	}

	std::string NormalizeTypeName(std::string value)
	{
		value = RemoveTypePrefix(value);
		while (EndsWith(value, "&") || EndsWith(value, "*"))
			value.pop_back();
		while (EndsWith(value, "[]"))
			value.resize(value.size() - 2);
		return RemoveGenericArguments(value);
	}

	uint16_t BuildFieldFlags(const CollectedField& field)
	{
		uint16_t flags = field.flags != 0 ? static_cast<uint16_t>(field.flags) : 0x0006;
		if (field.isStatic) flags |= 0x0010;
		if (field.isReadOnly) flags |= 0x0020;
		if (field.isLiteral) flags |= 0x0040;
		return flags;
	}

	uint16_t BuildParamFlags(const CollectedParam& param)
	{
		uint16_t flags = 0;
		if (param.isIn) flags |= 0x0001;
		if (param.isOut) flags |= 0x0002;
		return flags;
	}

	uint32_t BuildTypeFlags(const CollectedType& type)
	{
		uint32_t flags = type.flags != 0 ? type.flags : (type.isPublic ? 0x00000001u : 0u);
		if (type.kind == TypeKind::Interface) flags |= 0x00000020u | 0x00000080u;
		if (type.kind == TypeKind::Struct || type.kind == TypeKind::Enum) flags |= 0x00000100u;
		if (type.isAbstract) flags |= 0x00000080u;
		if (type.isSealed) flags |= 0x00000100u;
		if (type.isSerializable) flags |= 0x00002000u;
		if (type.kind != TypeKind::Interface) flags |= 0x00100000u;
		return flags;
	}

	uint16_t BuildMethodFlags(const CollectedMethod& method)
	{
		uint16_t flags = method.flags != 0 ? static_cast<uint16_t>(method.flags) : 0x0006;
		flags |= 0x0080;
		if (method.isStatic) flags |= 0x0010;
		if (method.isVirtual) flags |= 0x0040;
		if (method.isAbstract) flags |= 0x0400;
		if (method.isSealed) flags |= 0x0020;
		if (method.name == ".ctor" || method.name == ".cctor") flags |= 0x0800 | 0x1000;
		else flags = static_cast<uint16_t>(flags & ~0x1000u);
		return flags;
	}

	uint16_t BuildMethodImplFlags(const CollectedMethod& method)
	{
		return method.isAbstract ? 0 : 0;
	}

	uint8_t PrimitiveElementType(const std::string& typeName, bool allowVoid)
	{
		const std::string lower = ToLowerAscii(LastNameComponent(typeName));
		if (allowVoid && lower == "void") return 0x01;
		if (lower == "bool" || lower == "boolean") return 0x02;
		if (lower == "char") return 0x03;
		if (lower == "sbyte") return 0x04;
		if (lower == "byte") return 0x05;
		if (lower == "short" || lower == "int16") return 0x06;
		if (lower == "ushort" || lower == "uint16") return 0x07;
		if (lower == "int" || lower == "int32") return 0x08;
		if (lower == "uint" || lower == "uint32") return 0x09;
		if (lower == "long" || lower == "int64") return 0x0A;
		if (lower == "ulong" || lower == "uint64") return 0x0B;
		if (lower == "float" || lower == "single") return 0x0C;
		if (lower == "double") return 0x0D;
		if (lower == "string") return 0x0E;
		if (lower == "intptr") return 0x18;
		if (lower == "uintptr") return 0x19;
		if (lower == "object") return 0x1C;
		return 0;
	}
}
