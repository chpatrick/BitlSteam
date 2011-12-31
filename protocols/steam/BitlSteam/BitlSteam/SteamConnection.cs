using System;

using SteamKit2;

namespace BitlSteam
{
	public class SteamConnection
	{
		SteamClient client;
		SteamFriends friends;
		SteamUser user;
		
		IntPtr ic;
		
		public SteamConnection (IntPtr ic)
		{
			this.ic = ic;
			
			client = new SteamClient();
			
			friends = client.GetHandler<SteamFriends>();
			user = client.GetHandler<SteamUser>();
		}
		
		#region External functions
		public void Login(string username, string password, string authCode) {
			client.Connect();
			var callback = client.WaitForCallback();
			var connect = callback as ConnectCallback;
			if (connect == null || connect.Result != EResult.OK) {
				Fatal("Connection failed");
				return;
			}
			
			var credentials = new SteamUser.LogOnDetails {
				Username = username,
				Password = password,
				AuthCode = authCode,
			};
			
			user.LogOn(credentials);
			callback = client.WaitForCallback();
			LogOnCallback login = callback as LogOnCallback;
			if (login == null) {
				Fatal ("Login failed");
				return;
			}
			else switch (login.Result) {
				case EResult.OK: break;
				case EResult.AccountLogonDenied:
				case EResult.AccountLogonDeniedNoMailSent:
					Fatal ("Steam guard authentication needed");
					return;
				default:
					Fatal ("Login failed");
					return;
			}
			
			foreach (var friend in friends.GetFriends())
				NotifyAddedBuddy(friend);
		}
		
		public void SendMessage(string name, string message) {
			friends.SendChatMessage(friends.GetFriendID(name),
			                        EChatEntryType.ChatMsg,
			                        message);
		}
		
		public void Logout() {
			user.LogOff();
			client.Disconnect();
			NotifyDisconnected();
		}
		#endregion
		
		void Fatal(string reason) {
			NotifyError(reason);
			NotifyDisconnected(ic, true);
		}
		
		void HandleCallback() {
			var msg = client.GetCallback();
			
			if (msg == null)
				return;
			
			client.FreeLastCallback();
			
			msg.Handle<SteamFriends.FriendMsgCallback>( friendMsg => {
                switch (friendMsg.EntryType) {
					case EChatEntryType.ChatMsg:
					case EChatEntryType.Emote:
					case EChatEntryType.InviteGame:
						NotifyMessage(friendMsg.Sender, friendMsg.Message);
						break;
				}
			});
		}
		
		void NotifyError(string reason) {
			Native.imcb_error(reason);
		}
		
		void NotifyConnected() {
			Native.imc_connected(ic);
		}
		
		void NotifyDisconnected() {
			Native.imc_logout(ic);
		}
		
		void NotifyAddedBuddy(SteamID id) {
			Native.imcb_add_buddy(ic, friends.GetFriendPersonaName(id), null);
		}
		
		void NotifyRemovedBuddy(SteamID id) {
			Native.imcb_remove_buddy(ic, friends.GetFriendPersonaName(id), null);
		}
		
		void NotifyMessage(SteamID id, string message) {
			Native.steam_message(ic, friends.GetFriendPersonaName(id), message);
		}
	}
}

