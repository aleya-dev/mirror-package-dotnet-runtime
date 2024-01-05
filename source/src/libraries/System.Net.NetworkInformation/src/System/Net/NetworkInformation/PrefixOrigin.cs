// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.

namespace System.Net.NetworkInformation
{
    /// <summary>
    /// Specifies how an IP address network prefix was located.
    /// </summary>
    public enum PrefixOrigin
    {
        Other = 0,
        Manual,
        WellKnown,
        Dhcp,
        RouterAdvertisement,
    }
}
