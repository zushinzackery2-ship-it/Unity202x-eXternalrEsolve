#include "SmokeAssertions.h"

#include <format>
#include <iostream>

namespace OfflineBehavior
{

void CheckDumpCs(Checks& checks, const std::string& dump, const SmokeModule& module)
{
    std::cout << "-- dump.cs layout\n";
    checks.Contains(dump, "// Image 0: mscorlib.dll - 0", "image header line");
    checks.Contains(dump, "// Namespace: Smoke", "namespace comment");
    checks.Contains(dump, "[Serializable]", "serializable attribute");
    checks.Contains(dump, "[Marker(\"tagged\")]", "custom attribute rendered");
    checks.Contains(dump, std::format(
        "public sealed class Derived : BaseType, IDisposable // TypeDefIndex: {}",
        static_cast<int>(kTypeDerived)),
        "type declaration with base list and TypeDefIndex");
    checks.Contains(dump, "public abstract class BaseType // TypeDefIndex:", "abstract class declaration");
    checks.Contains(dump, "public interface IDisposable", "interface declaration");
    checks.Contains(dump, "protected internal class Derived.Nested", "nested visibility declaration");
    checks.Contains(dump, "\tprivate int _counter; // 0x10", "field offset comment");
    checks.Contains(dump, "\tpublic readonly string Tag; // 0x18", "readonly field");
    checks.Contains(dump, "\tpublic const int Answer = 42;", "const field with default");
    checks.Contains(dump, "\tpublic int[] Values; // 0x20", "SZARRAY field");
    checks.Contains(dump, "\tpublic int[,] Matrix; // 0x28", "ranked ARRAY field");
    checks.Contains(dump, "\tpublic Container<int> GenericValue; // 0x30", "generic class field");
    checks.Contains(dump, "\tpublic int UnknownOffset; // 0xFFFFFFFF", "missing offset sentinel");
    checks.Contains(dump, "\tpublic int Counter { get; set; }", "property accessors");
    checks.Contains(dump, "\tpublic event IDisposable Changed;", "event declaration");
    checks.Contains(dump, "// RVA: -1 Offset: -1 Slot: 0", "abstract method has no address and keeps slot");
    checks.Contains(dump, std::format(
        "// RVA: 0x{:X} Offset: 0x{:X} VA: 0x{:X} Slot: 0",
        module.MethodRva(kMethodDerivedRun),
        module.MethodFileOffset(kMethodDerivedRun),
        module.MethodVa(kMethodDerivedRun)),
        "concrete method uses a real file offset");
    checks.Contains(dump, "public sealed override void Run() { }", "sealed override rendering");
    checks.Contains(dump, "public abstract void Run();", "abstract method ends with semicolon");
    checks.Contains(dump,
        "public static int Compute(int a, out int b, ref int c, in int d, string label = \"hi\")",
        "full signature with ref/out/in/default");
    checks.Contains(dump, "public U Echo<U>(U value)", "MVAR generic method signature");
    checks.Contains(dump, "/* GenericInstMethod :", "generic instantiation block");
    checks.Contains(dump, "|-Container<int>.Get", "generic instantiation entry");
    checks.NotContains(dump, std::format(
        "RVA: 0x{:X} Offset: 0x{:X} VA",
        module.MethodRva(kMethodDerivedRun),
        module.MethodRva(kMethodDerivedRun)),
        "Offset is not a copy of the RVA");
}

void CheckSidecars(Checks& checks, const std::string& outputDir)
{
    std::cout << "-- script.json / stringliteral.json / il2cpp.h\n";
    const std::string script = ReadTextFile(outputDir + "/script.json");
    checks.Ok(!script.empty(), "script.json written");
    checks.Contains(script, "\"ScriptMethod\"", "script.json has ScriptMethod");
    checks.Contains(script, "Derived$$Compute", "script.json names a method");

    const std::string literals = ReadTextFile(outputDir + "/stringliteral.json");
    checks.Ok(!literals.empty(), "stringliteral.json written");
    checks.Contains(literals, "hello smoke", "literal present in stringliteral.json");

    const std::string header = ReadTextFile(outputDir + "/il2cpp.h");
    checks.Ok(!header.empty(), "il2cpp.h written");
    checks.Contains(header, "struct Smoke_Derived_o", "il2cpp.h declares the type struct");
    checks.Contains(header, "struct Derived_Nested_o", "il2cpp.h declares the nested type struct");
}

} // namespace OfflineBehavior
