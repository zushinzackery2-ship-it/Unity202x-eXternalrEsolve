#pragma once

#include <er2/unity2/dumpsdk/offline/BinaryStream.h>
#include <er2/unity2/dumpsdk/offline/MetadataStructs.h>

#include <string>
#include <unordered_map>
#include <vector>

namespace er2
{

class Metadata : public BinaryStream
{
public:
    Metadata() = default;

    explicit Metadata(const uint8_t* data, size_t size);

    void Load(const uint8_t* data, size_t size);

    double Version() const
    {
        return version_;
    }

    const Il2CppGlobalMetadataHeader& Header() const
    {
        return header_;
    }

    const std::vector<Il2CppImageDefinition>& ImageDefs() const
    {
        return imageDefs_;
    }

    const std::vector<Il2CppTypeDefinition>& TypeDefs() const
    {
        return typeDefs_;
    }

    const std::vector<Il2CppMethodDefinition>& MethodDefs() const
    {
        return methodDefs_;
    }

    const std::vector<Il2CppFieldDefinition>& FieldDefs() const
    {
        return fieldDefs_;
    }

    const std::vector<Il2CppParameterDefinition>& ParameterDefs() const
    {
        return parameterDefs_;
    }

    const std::vector<Il2CppPropertyDefinition>& PropertyDefs() const
    {
        return propertyDefs_;
    }

    const std::vector<Il2CppStringLiteral>& StringLiterals() const
    {
        return stringLiterals_;
    }

    std::string GetStringFromIndex(uint32_t index) const;
    std::string GetStringLiteralFromIndex(uint32_t index) const;

private:
    Metadata CreateBoundView() const;

    void Parse();
    void DetectVersion24Variants();
    void ReadTables();

    template<typename T>
    std::vector<T> ReadMetadataTable(uint32_t offset, int32_t byteSize, size_t elementSize) const;

    Il2CppGlobalMetadataHeader ReadGlobalMetadataHeader() const;
    Il2CppImageDefinition ReadImageDefinition();
    Il2CppTypeDefinition ReadTypeDefinition();
    Il2CppMethodDefinition ReadMethodDefinition();
    Il2CppParameterDefinition ReadParameterDefinition();
    Il2CppFieldDefinition ReadFieldDefinition();
    Il2CppPropertyDefinition ReadPropertyDefinition();
    Il2CppStringLiteral ReadStringLiteral();

    size_t ImageDefinitionSize() const;
    size_t TypeDefinitionSize() const;
    size_t MethodDefinitionSize() const;
    size_t ParameterDefinitionSize() const;
    size_t FieldDefinitionSize() const;
    size_t PropertyDefinitionSize() const;

    static bool VersionInRange(double version, double minVersion, double maxVersion, bool hasMin, bool hasMax);

    double version_ = 0.0;
    Il2CppGlobalMetadataHeader header_{};
    std::vector<Il2CppImageDefinition> imageDefs_;
    std::vector<Il2CppTypeDefinition> typeDefs_;
    std::vector<Il2CppMethodDefinition> methodDefs_;
    std::vector<Il2CppFieldDefinition> fieldDefs_;
    std::vector<Il2CppParameterDefinition> parameterDefs_;
    std::vector<Il2CppPropertyDefinition> propertyDefs_;
    std::vector<Il2CppStringLiteral> stringLiterals_;
    mutable std::unordered_map<uint32_t, std::string> stringCache_;
};

} // namespace er2
