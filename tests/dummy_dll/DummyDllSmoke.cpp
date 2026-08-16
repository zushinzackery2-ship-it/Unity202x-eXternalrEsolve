#include <er2/unity2/dumpsdk/writers/dummy_dll_generator.hpp>

#include <filesystem>
#include <fstream>
#include <iostream>

namespace
{

er2::CollectedType BuildType()
{
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
    return type;
}

er2::CollectedData BuildData()
{
    const er2::CollectedType type = BuildType();
    er2::CollectedData data;
    const char* names[] = {
        "SmokeAssembly",
        "Assembly-CSharp.dll",
        "UnityEngine.CoreModule.dll",
        "System.Core",
    };
    for (const char* name : names)
    {
        er2::CollectedAssembly assembly;
        assembly.name = name;
        assembly.fileName = name;
        assembly.types.push_back(type);
        data.assemblies.push_back(assembly);
    }
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

    const char* expected[] = {
        "SmokeAssembly.dll",
        "Assembly-CSharp.dll",
        "UnityEngine.CoreModule.dll",
        "System.Core.dll",
    };
    for (const char* name : expected)
    {
        const std::filesystem::path output = std::filesystem::path("out/DummyDll") / name;
        if (!std::filesystem::is_regular_file(output) || !HasPortableExecutableHeader(output))
        {
            std::cerr << "Generated assembly is missing or invalid: " << name << "\n";
            return 2;
        }
    }

    if (std::filesystem::exists("out/DummyDll/Assembly-CSharp.dll.dll")
        || std::filesystem::exists("out/DummyDll/UnityEngine.CoreModule.dll.dll"))
    {
        std::cerr << "Generated assembly kept a duplicated .dll suffix\n";
        return 3;
    }

    std::cout << "DummyDllSmoke passed\n";
    return 0;
}
