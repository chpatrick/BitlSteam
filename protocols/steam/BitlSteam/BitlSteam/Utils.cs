using System;
using System.Collections.Generic;
using System.Linq;

using SteamKit2;

namespace BitlSteam
{
	public static class Utils
	{
		public static IEnumerable<SteamID> GetFriends(this SteamFriends friends) {
			// yuk
			for (int i = 0; i < friends.GetFriendCount(); i++)
				yield return friends.GetFriendByIndex(i);
		}
		
		public static SteamID GetFriendID(this SteamFriends friends, string name) {
			return friends
				   .GetFriends()
				   .FirstOrDefault(id => friends.GetFriendPersonaName(id) == name);
		}
	}
}

