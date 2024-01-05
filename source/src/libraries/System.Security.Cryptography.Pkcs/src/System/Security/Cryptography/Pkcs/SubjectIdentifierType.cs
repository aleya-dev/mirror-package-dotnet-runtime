// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.

using System;
using System.Diagnostics;

namespace System.Security.Cryptography.Pkcs
{
    public enum SubjectIdentifierType
    {
        Unknown = 0,                // Use any of the following as appropriate
        IssuerAndSerialNumber = 1,  // X509IssuerSerial
        SubjectKeyIdentifier = 2,   // SKI hex string
        NoSignature = 3             // NoSignature
    }
}
