#include <er2/unity2/dumpsdk/writers/cli/UnityCliMetadataBuilder.h>

namespace er2::UnityCli
{
	void MetadataBuilder::AddFields(const CollectedType& type)
	{
		for (size_t i = 0; i < type.fields.size(); ++i)
		{
			const CollectedField& field = type.fields[i];
			FieldRow row;
			row.Flags = BuildFieldFlags(field);
			row.Name = heaps_.GetString(CleanMetadataName(field.name, "field_" + std::to_string(i)));
			row.Signature = heaps_.AddBlob(BuildFieldSignature(field.typeName));
			rows_.Fields.push_back(row);
		}
	}

	std::unordered_map<std::string, uint32_t> MetadataBuilder::AddMethods(const CollectedType& type)
	{
		std::unordered_map<std::string, uint32_t> methodRows;
		for (size_t i = 0; i < type.methods.size(); ++i)
		{
			const CollectedMethod& method = type.methods[i];
			const uint32_t paramStart = static_cast<uint32_t>(rows_.Params.size()) + 1;
			for (size_t paramIndex = 0; paramIndex < method.params.size(); ++paramIndex)
				AddParam(method.params[paramIndex], static_cast<uint16_t>(paramIndex + 1));

			MethodDefRow row;
			row.ImplFlags = BuildMethodImplFlags(method);
			row.Flags = BuildMethodFlags(method);
			row.Rva = method.isAbstract ? 0 : AddMethodBody(method.returnType);
			row.Name = heaps_.GetString(CleanMetadataName(method.name, "Method_" + std::to_string(i)));
			row.Signature = heaps_.AddBlob(BuildMethodSignature(method));
			row.ParamList = paramStart;
			rows_.Methods.push_back(row);
			methodRows.try_emplace(method.name, static_cast<uint32_t>(rows_.Methods.size()));
		}
		return methodRows;
	}

	void MetadataBuilder::AddParam(const CollectedParam& param, uint16_t sequence)
	{
		ParamRow row;
		row.Flags = BuildParamFlags(param);
		row.Sequence = sequence;
		row.Name = heaps_.GetString(CleanMetadataName(param.name, "param_" + std::to_string(sequence)));
		rows_.Params.push_back(row);
	}

	void MetadataBuilder::AddProperties(uint32_t parentTypeDef, const CollectedType& type, std::unordered_map<std::string, uint32_t>& methodRows)
	{
		if (type.properties.empty())
			return;

		PropertyMapRow map;
		map.Parent = parentTypeDef;
		map.PropertyList = static_cast<uint32_t>(rows_.Properties.size()) + 1;
		rows_.PropertyMaps.push_back(map);

		for (size_t i = 0; i < type.properties.size(); ++i)
		{
			const CollectedProperty& property = type.properties[i];
			PropertyRow row;
			row.Name = heaps_.GetString(CleanMetadataName(property.name, "Property_" + std::to_string(i)));
			row.Type = heaps_.AddBlob(BuildPropertySignature(property.typeName));
			rows_.Properties.push_back(row);

			const uint32_t propertyRow = static_cast<uint32_t>(rows_.Properties.size());
			if (property.hasGetter)
			{
				const uint32_t methodRow = ResolvePropertyMethod(methodRows, "get_" + property.name, property, false);
				if (methodRow != 0)
					AddMethodSemantics(0x0002, methodRow, propertyRow);
			}
			if (property.hasSetter)
			{
				const uint32_t methodRow = ResolvePropertyMethod(methodRows, "set_" + property.name, property, true);
				if (methodRow != 0)
					AddMethodSemantics(0x0001, methodRow, propertyRow);
			}
		}
	}

	uint32_t MetadataBuilder::ResolvePropertyMethod(std::unordered_map<std::string, uint32_t>& methodRows, const std::string& methodName, const CollectedProperty& property, bool isSetter)
	{
		const auto existing = methodRows.find(methodName);
		if (existing != methodRows.end())
			return existing->second;

		(void)property;
		(void)isSetter;
		return 0;
	}

	void MetadataBuilder::AddMethodSemantics(uint16_t semantics, uint32_t methodRow, uint32_t propertyRow)
	{
		MethodSemanticsRow row;
		row.Semantics = semantics;
		row.Method = methodRow;
		row.Association = propertyRow;
		rows_.MethodSemantics.push_back(row);
	}

	uint32_t MetadataBuilder::AddMethodBody(const std::string& returnType)
	{
		methodBodies_.AlignTo(4);
		const uint32_t rva = 0x2000 + 72 + static_cast<uint32_t>(methodBodies_.Size());
		const std::string normalized = NormalizeTypeName(returnType);
		const uint8_t primitive = PrimitiveElementType(normalized, true);

		if (primitive == 0x01)
		{
			methodBodies_.WriteU8(0x02);
			methodBodies_.WriteU8(0x2A);
		}
		else if (primitive == 0x0A || primitive == 0x0B)
		{
			methodBodies_.WriteU8(0x0E);
			methodBodies_.WriteU8(0x16);
			methodBodies_.WriteU8(0x6A);
			methodBodies_.WriteU8(0x2A);
		}
		else if (primitive == 0x0C)
		{
			methodBodies_.WriteU8(0x1A);
			methodBodies_.WriteU8(0x22);
			methodBodies_.WriteU32(0);
			methodBodies_.WriteU8(0x2A);
		}
		else if (primitive == 0x0D)
		{
			methodBodies_.WriteU8(0x2A);
			methodBodies_.WriteU8(0x23);
			methodBodies_.WriteU64(0);
			methodBodies_.WriteU8(0x2A);
		}
		else if (primitive >= 0x02 && primitive <= 0x09)
		{
			methodBodies_.WriteU8(0x0A);
			methodBodies_.WriteU8(0x16);
			methodBodies_.WriteU8(0x2A);
		}
		else
		{
			methodBodies_.WriteU8(0x0A);
			methodBodies_.WriteU8(0x14);
			methodBodies_.WriteU8(0x2A);
		}

		return rva;
	}
}
