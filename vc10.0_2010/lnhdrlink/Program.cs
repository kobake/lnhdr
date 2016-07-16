using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Windows.Forms;
using System.Runtime.InteropServices;
using System.IO;

namespace lnhdrlink
{
	class Program
	{
		[DllImport("kernel32.dll")]
		static extern uint GetLastError();

		const uint ERROR_PRIVILEGE_NOT_HELD = 0x522;

		[DllImport("kernel32.dll", CharSet = CharSet.Auto)]
		static extern bool CreateSymbolicLink(
			string lpSymlinkFileName,
			string lpTargetFileName,
			SymbolicLink dwFlags
		);

		enum SymbolicLink
		{
			File = 0,
			Directory = 1
		}


		static void Main(string[] args)
		{
			try
			{
				// runas option
				int iarg = 0;
				bool runas = (args[iarg] == "/runas");
				if (runas) iarg++;

				// file paths
				string linkPath = args[iarg++];
				string filePath = args[iarg++];

				// file symlink
				if (File.Exists(filePath))
				{
					bool b = CreateSymbolicLink(linkPath, filePath, SymbolicLink.File);
					uint error = GetLastError(); // ※bがtrueでもERROR_PRIVILEGE_NOT_HELDになることがある.
					if (error == ERROR_PRIVILEGE_NOT_HELD && !runas)
					{
						RunAsAdministrator(args);
					}
				}
				// directory symlink
				else if (Directory.Exists(filePath))
				{
					bool b = CreateSymbolicLink(linkPath, filePath, SymbolicLink.Directory);
					uint error = GetLastError(); // ※bがtrueでもERROR_PRIVILEGE_NOT_HELDになることがある.
					if (error == ERROR_PRIVILEGE_NOT_HELD && !runas)
					{
						RunAsAdministrator(args);
					}
				}
				// other: error
				else
				{
					Environment.Exit(1);
				}
			}
			catch (ArgumentOutOfRangeException ex)
			{
				Logger.Log(ex.Message);
			}
		}

		// 管理者として実行.
		static void RunAsAdministrator(string[] args)
		{
			System.Diagnostics.ProcessStartInfo psi = new System.Diagnostics.ProcessStartInfo();
			psi.UseShellExecute = true;
			psi.FileName = Application.ExecutablePath;
			psi.Arguments = "/runas";
			foreach (string arg in args)
			{
				psi.Arguments += " \"" + arg + "\"";
			}
			psi.Verb = "runas";
			try
			{
				System.Diagnostics.Process.Start(psi);
			}
			catch (System.ComponentModel.Win32Exception ex)
			{
				// UACダイアログキャンセル等.
				Logger.Log(ex.Message);
			}
		}
	}
}
