#pragma once

#include <er2/unity2/dumpsdk/writers/cli/UnityCliWriter.h>

#include <string_view>
#include <unordered_map>

namespace er2::UnityCli
{
	struct LocalTypeInfo
	{
		uint32_t RowIndex = 0;
		TypeKind Kind = TypeKind::Class;
	};

	std::string ToLowerAscii(std::string value);
	bool EndsWith(const std::string& value, std::string_view suffix);
	std::string StripExtension(std::string value);
	std::string CleanMetadataName(std::string value, const std::string& fallback);
	std::string RemoveTypePrefix(std::string value);
	std::string RemoveGenericArguments(std::string value);
	std::string LastNameComponent(const std::string& value);
	std::string NormalizeTypeName(std::string value);
	uint16_t BuildFieldFlags(const CollectedField& field);
	uint16_t BuildParamFlags(const CollectedParam& param);
	uint32_t BuildTypeFlags(const CollectedType& type);
	uint16_t BuildMethodFlags(const CollectedMethod& method);
	uint16_t BuildMethodImplFlags(const CollectedMethod& method);
	uint8_t PrimitiveElementType(const std::string& typeName, bool allowVoid);

	class MetadataBuilder
	{
	public:
		bool Build(const CollectedAssembly& assembly, std::vector<uint8_t>& metadata, std::vector<uint8_t>& methodBodies);

	private:
		void InitializeAssembly(const CollectedAssembly& assembly);
		std::array<uint8_t, 16> BuildGuid(const std::string& value);
		uint32_t AddTypeRef(const std::string& namespaceName, const std::string& typeName);
		void RegisterTypeDefinitions(const CollectedAssembly& assembly);
		void RegisterLocalType(const std::string& name, const std::string& namespaceName, uint32_t rowIndex, TypeKind kind);
		void PopulateTypeMembers(const CollectedAssembly& assembly);
		uint32_t BuildExtends(const CollectedType& type);
		void AddFields(const CollectedType& type);
		std::unordered_map<std::string, uint32_t> AddMethods(const CollectedType& type);
		void AddParam(const CollectedParam& param, uint16_t sequence);
		void AddProperties(uint32_t parentTypeDef, const CollectedType& type, std::unordered_map<std::string, uint32_t>& methodRows);
		uint32_t ResolvePropertyMethod(std::unordered_map<std::string, uint32_t>& methodRows, const std::string& methodName, const CollectedProperty& property, bool isSetter);
		void AddMethodSemantics(uint16_t semantics, uint32_t methodRow, uint32_t propertyRow);
		uint32_t AddMethodBody(const std::string& returnType);
		std::vector<uint8_t> BuildFieldSignature(const std::string& typeName);
		std::vector<uint8_t> BuildPropertySignature(const std::string& typeName);
		std::vector<uint8_t> BuildMethodSignature(const CollectedMethod& method);
		void WriteType(ByteWriter& signature, const std::string& rawTypeName, bool allowVoid, bool forceByRef);
		const LocalTypeInfo* FindLocalType(const std::string& typeName) const;

		MetadataHeaps heaps_;
		MetadataRows rows_;
		ByteWriter methodBodies_;
		uint32_t objectRef_ = 0;
		uint32_t valueTypeRef_ = 0;
		uint32_t enumRef_ = 0;
		uint32_t delegateRef_ = 0;
		std::unordered_map<std::string, uint32_t> typeRefs_;
		std::unordered_map<std::string, LocalTypeInfo> localTypes_;
	};
}
