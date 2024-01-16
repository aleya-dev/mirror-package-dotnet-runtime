try
{
    &Push-Location -Path source
    &.\build.cmd -s Host.Native -c debug
    &.\build.cmd -s Host.Native -c release
}
finally
{
    Pop-Location
}

&conan export-pkg . -s os="Windows" -s arch="x86_64" -s build_type="Debug" --user aleya --channel public
&conan export-pkg . -s os="Windows" -s arch="x86_64" -s build_type="Release"  --user aleya --channel public
