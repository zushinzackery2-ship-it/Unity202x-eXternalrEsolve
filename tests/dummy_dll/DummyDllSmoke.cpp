#include <er2/unity2/dumpsdk/writers/dummy_dll_generator.hpp>

#include <filesystem>
#include <fstream>
#include <iostream>

namespace
{

er2::CollectedData BuildData()
{
    er2::CollectedData data;
    er2::CollectedAssembly assembly;
    assembly.name = "SmokeAssembly";
    assembly.fileName = "SmokeAssembly.dll";

    er2::CollectedType type;
    type.name = "SmokeType";
    type.namespaceName = "Smoke.Namespace";
    type.kind = er2::TypeKind::Class;
    type.isPublic = true;

    er2::CollectedField field;
    field.name = "Value";
    field.typeName = "Int32";
    field.flags = 0x0006;
    type.fields.push_back(field);

    er2::CollectedProperty property;
    property.name = "Name";
    property.typeName = "String";
    property.hasGetter = true;
    property.hasSetter = true;
    type.properties.push_back(property);

    er2::CollectedMethod method;
    method.name = "Add";
    method.returnType = "Int32";
    method.flags = 0x0006;

    er2::CollectedParam left;
    left.name = "left";
    left.typeName = "Int32";
    method.params.push_back(left);

    er2::CollectedParam right;
    right.name = "right";
    right.typeName = "Int32";
    method.params.push_back(right);
    type.methods.push_back(method);

    assembly.types.push_back(type);
    data.assemblies.push_back(assembly);
    return data;
}

bool HasPortableExecutableHeader(const std::filesystem::path& path)
{
    std::ifstream input(path, std::ios::binary);
    char signature[2] = {};
    input.read(signature, sizeof(signature));
    return input.gcount() == sizeof(signature)
        && signature[0] == 'M'
        && signature[1] == 'Z';
}

} // namespace

int main()
{
    std::filesystem::remove_all("out");
    const er2::CollectedData data = BuildData();
    if (!er2::DummyDllGenerator::Generate("out", data))
    {
        std::cerr << "DummyDllGenerator failed\n";
        return 1;
    }

    const std::filesystem::path output = "out/DummyDll/SmokeAssembly.dll";
    if (!std::filesystem::is_regular_file(output) || !HasPortableExecutableHeader(output))
    {
        std::cerr << "Generated assembly is missing or invalid\n";
        return 2;
    }

    std::cout << "DummyDllSmoke passed\n";
    return 0;
}
