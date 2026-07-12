#include <er2/unity2/dumpsdk/writers/cli/UnityCliMetadataBuilder.h>

namespace er2::UnityCli
{
	std::vector<uint8_t> MetadataBuilder::BuildFieldSignature(const std::string& typeName)
	{
		ByteWriter signature;
		signature.WriteU8(0x06);
		WriteType(signature, typeName, false, false);
		return signature.Data();
	}

	std::vector<uint8_t> MetadataBuilder::BuildPropertySignature(const std::string& typeName)
	{
		ByteWriter signature;
		signature.WriteU8(0x28);
		WriteCompressedUInt(signature, 0);
		WriteType(signature, typeName, false, false);
		return signature.Data();
	}

	std::vector<uint8_t> MetadataBuilder::BuildMethodSignature(const CollectedMethod& method)
	{
		ByteWriter signature;
		signature.WriteU8(method.isStatic ? 0x00 : 0x20);
		WriteCompressedUInt(signature, static_cast<uint32_t>(method.params.size()));
		WriteType(signature, method.returnType, true, false);
		for (const CollectedParam& param : method.params)
			WriteType(signature, param.typeName, false, param.isByRef || param.isOut);
		return signature.Data();
	}

	void MetadataBuilder::WriteType(ByteWriter& signature, const std::string& rawTypeName, bool allowVoid, bool forceByRef)
	{
		std::string typeName = RemoveTypePrefix(rawTypeName);
		bool byRef = forceByRef;
		if (EndsWith(typeName, "&"))
		{
			byRef = true;
			typeName.pop_back();
		}

		if (byRef)
			signature.WriteU8(0x10);

		uint32_t arrayDepth = 0;
		while (EndsWith(typeName, "[]"))
		{
			++arrayDepth;
			typeName.resize(typeName.size() - 2);
		}
		for (uint32_t i = 0; i < arrayDepth; ++i)
			signature.WriteU8(0x1D);

		typeName = RemoveGenericArguments(typeName);
		if (EndsWith(typeName, "*"))
		{
			signature.WriteU8(0x18);
			return;
		}

		const uint8_t primitive = PrimitiveElementType(typeName, allowVoid);
		if (primitive != 0)
		{
			signature.WriteU8(primitive);
			return;
		}

		const std::string normalized = NormalizeTypeName(typeName);
		if (const LocalTypeInfo* local = FindLocalType(normalized))
		{
			signature.WriteU8(local->Kind == TypeKind::Struct || local->Kind == TypeKind::Enum ? 0x11 : 0x12);
			WriteCompressedUInt(signature, EncodeTypeDefOrRef(local->RowIndex, TableId::TypeDef));
			return;
		}

		if (normalized.empty())
		{
			signature.WriteU8(0x1C);
			return;
		}

		std::string namespaceName;
		std::string name = normalized;
		const size_t dot = normalized.find_last_of('.');
		if (dot != std::string::npos)
		{
			namespaceName = normalized.substr(0, dot);
			name = normalized.substr(dot + 1);
		}

		signature.WriteU8(0x12);
		WriteCompressedUInt(signature, EncodeTypeDefOrRef(AddTypeRef(namespaceName, CleanMetadataName(name, "Object")), TableId::TypeRef));
	}

	const LocalTypeInfo* MetadataBuilder::FindLocalType(const std::string& typeName) const
	{
		const auto exact = localTypes_.find(typeName);
		if (exact != localTypes_.end())
			return &exact->second;

		const std::string last = LastNameComponent(typeName);
		const auto simple = localTypes_.find(last);
		if (simple != localTypes_.end())
			return &simple->second;

		return nullptr;
	}
}
