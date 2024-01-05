// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.

public static class Library
{
    private static string s_name
#if NoCCtor
 = "Library";
#else
    ;
    static Library()
    {
        s_name = "Library";
    }
#endif

    public static string Name { get { return s_name; } }
}
