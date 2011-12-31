using System;
using System.Runtime.InteropServices;

namespace BitlSteam
{
	public static class Native
	{
		[DllImport("__Internal")]
		public static extern void imcb_error(string message);
		
		[DllImport("__Internal")]
		public static extern void imc_logout(IntPtr ic, bool allowReconnect);
		
		[DllImport("__Internal")]
		public static extern void imc_connected(IntPtr ic);
		
		[DllImport("__Internal")]
		public static extern void imcb_add_buddy(IntPtr ic, string name, string @group);
		
		[DllImport("__Internal")]
		public static extern void imcb_remove_buddy(IntPtr ic, string name, string @group);
		
		[DllImport("__Internal")]
		public static extern void steam_receive_message(IntPtr ic, string name, string message);
	}
}

