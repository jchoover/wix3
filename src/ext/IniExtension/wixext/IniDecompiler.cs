//-------------------------------------------------------------------------------------------------
// <copyright file="IniDecompiler.cs" company="Outercurve Foundation">
//   Copyright (c) 2004, Outercurve Foundation.
//   This software is released under Microsoft Reciprocal License (MS-RL).
//   The license and further copyright text can be found in the file
//   LICENSE.TXT at the root directory of the distribution.
// </copyright>
// 
// <summary>
// The decompiler for the Windows Installer XML Toolset Ini Extension.
// </summary>
//-------------------------------------------------------------------------------------------------

namespace Microsoft.Tools.WindowsInstallerXml.Extensions
{
    using System;
    using System.Collections;
    using System.Diagnostics;
    using System.Globalization;

    using Ini = Microsoft.Tools.WindowsInstallerXml.Extensions.Serialize.Ini;
    using Wix = Microsoft.Tools.WindowsInstallerXml.Serialize;

    /// <summary>
    /// The decompiler for the Windows Installer XML Toolset Ini Extension.
    /// </summary>
    public sealed class IniDecompiler : DecompilerExtension
    {
        /// <summary>
        /// Decompiles an extension table.
        /// </summary>
        /// <param name="table">The table to decompile.</param>
        public override void DecompileTable(Table table)
        {
            switch (table.Name)
            {
                case "WixIniSearch":
                    this.DecompileWixIniSearchTable(table);
                    break;
                default:
                    base.DecompileTable(table);
                    break;
            }
        }

        /// <summary>
        /// Decompile the WixIniSearch table.
        /// </summary>
        /// <param name="table">The table to decompile.</param>
        private void DecompileWixIniSearchTable(Table table)
        {
            foreach (Row row in table.Rows)
            {
                Ini.IniSearchEx search = new Ini.IniSearchEx();

                search.Id = (string)row[0];                
                search.Name = (string)row[2];
                search.Section = (string)row[3];
                search.Key = (string)row[4];

                Wix.Property property = (Wix.Property)this.Core.GetIndexedElement("Property", (string)row[1]);
                if (null != property)
                {
                    property.AddChild(search);
                }
                else
                {
                    this.Core.OnMessage(WixWarnings.ExpectedForeignRow(row.SourceLineNumbers, table.Name, row.GetPrimaryKey(DecompilerCore.PrimaryKeyDelimiter), "File_", (string)row[1], "File"));
                }
            }
        }
    }
}
