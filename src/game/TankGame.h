// This file is part of Tread Marks
// 
// Tread Marks is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
// 
// Tread Marks is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
// 
// You should have received a copy of the GNU General Public License
// along with Tread Marks.  If not, see <http://www.gnu.org/licenses/>.

#ifndef TANKGAME_H
#define TANKGAME_H

#include "VoxelWorld.h"
#include "GenericBuffer.h"
#include "SimpleStatus.h"
#include "StatsPackage.h"
#include "LadderManager.h"
#include "Heartbeat.h"
#include "CJoyInput.h"
#include "PacketProcessors.h"
#include "TextLine.hpp"
#include <stdint.h>

enum MapType {MapTypeRace = 0, MapTypeDeathMatch, MapTypeTraining};

struct MapInfo{
	CStr title, file;
	MapType maptype;
	int32_t MapID;
	int32_t GameType;
	int32_t Laps;
	int32_t AITanks;
	int32_t TimeLimit;
	int32_t FragLimit;
	float EnemySkill;
	int32_t TankTypes;
	int32_t StartDelay;
	int32_t Mirror;
	int32_t AllowJoins;
	int32_t DisableFrags;
	int32_t NumTeams;
	int32_t TeamScores;
	int32_t MaxMapTeams;
	int32_t TeamDamage;
};

struct ServerEntryEx : public ServerEntry {
	CStr MapTitle;
	int32_t PingTime, PingCount;
	ServerEntryEx() : PingTime(0), PingCount(0) { };
};

enum ControlEntryID{
	CEID_None = 0, CEID_Left, CEID_Right, CEID_Up, CEID_Down, CEID_TLeft, CEID_TRight, CEID_Fire,
	CEID_Chat, CEID_TeamChat, CEID_Scores, CEID_FreeLook, CEID_GunToFront, CEID_GunToBack, CEID_GunToCam,
	CEID_TurretCam, CEID_TiltCamUp, CEID_TiltCamDn, CEID_SpinCamLt, CEID_SpinCamRt, CEID_SpinCamBk, CEID_CamReset
};

struct ControlEntry{
	CStr name, ctrl1, ctrl2, ctrl3;
	ControlEntryID ctlid;
	void Set(const char *n, const char *c1, const char *c2, const char *c3, ControlEntryID id){
		name = n;  ctrl1 = c1;  ctrl2 = c2;  ctrl3 = c3; ctlid = id;
	};
};

struct TeamInfo{
	ClassHash hash;	//Insignia type hash.
	CStr name;
	int32_t teamok;
};
#define MaxTeams 32

enum MasterSortMode{
	MSM_Name,
	MSM_Map,
	MSM_Mode,
	MSM_Time,
	MSM_Players,
	MSM_Ping
};

struct MasterServer{
	CStr address;
	sockaddr_in ip;
	MasterServer(){
		ip.sin_family = AF_INET;
		ip.sin_addr.s_addr = 0;
	};
};
#define MAX_MASTERS 10

struct CStrLink : public CStr, public LinklistBase<CStrLink> {
	CStrLink(){ };
	CStrLink(const char *c) : CStr(c) { };
};

struct TankInfo{
	CStr title, type;
	ClassHash hash;
	int32_t liquid;
};

#define MAP_SNAP_SIZE 256 //Width and height of map snapshot files.

enum GameTypeID {RaceType = 0, DeathmatchType, TutorialType};

	#define SEND_STATUS_TIME 1000
	#define NumControls 21
	#define MaxTankNames 512
	#define MaxChatLines 100
	#define MaxMusicFiles 100
	#define MaxTankTypes 100
	#define MaxMaps 128

struct CGraphicsSettings
{
	int32_t RendFlags;
	bool Stretch;
	bool UseFullScreen;
	int32_t DisableMT;
	bool UseFog;
	bool UseAlphaTest;
	int32_t MenuFire;
	int32_t GLBWIDTH;
	int32_t GLBHEIGHT;

	float Quality;
	bool WireFrame;
	bool StripFanMap;
	bool TreadMarks;
	bool DetailTerrain;
	int32_t PolyLod;
	float Particles;

	int32_t MaxTexRes;
	bool TexCompress;
	bool TexCompressAvail;
	int32_t Trilinear;
	bool PalettedTextures;

	int32_t DebugLockPatches;
	bool DebugPolyNormals;

	float ViewDistance;

	CGraphicsSettings();
};

struct CInputSettings
{
	float MouseSpeed;
	int32_t InvMouseY;
	int32_t MouseEnabled;
	int32_t AxisMapping[K_MAXJOYAXES + 2];
	ControlEntry Controls[NumControls];
	int32_t FirstCamControl;
	int32_t UseJoystick;
	CStr sStickName;
	int32_t StickID;
	float DeadZone;

	CInputSettings();
	void InitControls();
};

struct CGameSettings // This information is set at load time or changed with menus
{
	CInputSettings InputSettings;
	CGraphicsSettings GraphicsSettings;
	int32_t HowitzerTimeStart;
	float HowitzerTimeScale;
	int32_t ShowMPH;
	float EnemySkill;
	int32_t ServerRate;
	bool LogFileActive;
	CStr LogFileName;
	int32_t CamStyle;
	int32_t BypassIntro;
	int32_t LimitTankTypes;	//All, Steel, Liquid.
	int32_t TeamPlay;
	int32_t TeamDamage;
	int32_t TeamScores;
	int32_t NumTeams;
	int32_t TimeLimit;
	int32_t FragLimit;
	CStr AIPrefix;
	int32_t HiFiSound;
	int32_t ClientPort;
	int32_t ServerPort;
	int32_t MasterPort;
	int32_t StartDelay;
	bool MirroredMap;
	int32_t DediMapMode;	//1 is Randam.
	int32_t CoolDownTime;
	int32_t MaxClients;
	bool DedicatedServer;
	int32_t DedicatedFPS;
	int32_t MaxFPS;
	int32_t SendHeartbeats;	//Heartbeats to Masters.
	CStr ServerName;
	CStr ServerWebSite;
	CStr ServerInfo;
	CStr ServerCorrectedIP; // If the master server misidentifies your IP address (eg, master and client are behind the same firewall) you can set this to correct it.
	CStr MasterAddress;	//The single master that the client is connecting to.
	int32_t MasterPingsPerSecond;
	int32_t MasterPings;
	int32_t ClientRate;
	int32_t AllowSinglePlayerJoins;
	CStr InsigniaType;
	CStr TankType;
	CStr MapFile;
	CStr AnimationDir;
	int32_t AnimFPS;
	bool PlayMusic;
	float SoundVol, MusicVol;
	int32_t Laps;
	int32_t Deathmatch;
	CStr ServerAddress;
	CStr PlayerName;
	CStr OptAITankTeam;
	int32_t AIAutoFillCount;
	int32_t AIAutoFill;
	bool AutoStart;

	CGameSettings();
};

struct CGameState // This information can change from frame to frame
{
	int32_t HowitzerTime;
	int32_t PauseGame;
	EntityGID CurrentLeader, LastLeader;
	bool LadderMode;	//This is on if we are in ladder play mode.
	int32_t CurDediMap;	//So first map in cycle is 0.
	int32_t ReStartMap;
	int32_t CoolDownCounter;
	int32_t SomeoneWonRace;
	bool DemoMode;	//This is set when we are in automatic demo mode, so we can restart the game and stay in demo mode.
	float ViewAngle;
	int32_t sendstatuscounter;
	int32_t CountdownTimer;
	bool WritingAnim;
	int32_t TakeMapSnapshot;	//When set, causes a one-off power of 2 snapshot of the game to be taken and saved with the current map's name.
	int32_t AnimFrame;
	bool AutoDrive;
	int32_t NumAITanks;
	bool FPSCounter;
	bool Quit;
	int32_t ShowPlayerList;
	bool ToggleMenu;
	bool ActAsServer;
	CStr ConnectedToAddress;
	int32_t PauseScreenOn;
	int32_t KillSelf;
	bool SwitchSoundMode;
	ClassHash TeamHash;
	CStr MasterError;
	int32_t MasterPingIter;
	MasterSortMode SortMode;
	bool SwitchMode;
	CStr sMusicFile;
	float FPS;
	int32_t NetCPSOut, NetCPSIn;	//Networking stats.
	uint32_t LastNetBytesOut, LastNetBytesIn;

	CGameState();
};

struct CInputState
{
	int32_t GunTo;
	int32_t TurretCam;
	int32_t PointCamUD;
	int32_t PointCamLR;
	int32_t PointCamBK;
	int32_t ResetCam;
	int32_t FreeLook;
	CJoyInput input;
	int32_t TurnLR, MoveUD, AltUD, StrafeLR;
	int32_t TreadL, TreadR;
	int32_t SpaceBar;
	float MouseLR, MouseUD;

	CInputState();
};

struct CDediMap
{
	CStr FileName;
	int32_t Index;

	CDediMap() {Index = -1;}
};

class EntityBase;
class EntityTankGod;

class CTankGame
{
public:
	TankPacketProcessor				TankPacket;
	MasterClientPacketProcessor		MasterPacket;

	CGameSettings	GameSettings;
	CGameState		GameState;
	CInputState		InputState;
	SimpleStatus	LoadingStatus;


	EntityBase		*PlayerEnt;
	EntityTankGod	*GodEnt;

	Timer			tmr, tmr2;
	int32_t				frames;
	int32_t				framems, polyms, voxelms, thinkms, blitms;
	int32_t				LastSecs;

	float			ClientPacketRate;

	ConfigFile		cfg;

	int32_t				CurChatLine;
	CStr			ChatLine[MaxChatLines];
	int32_t				ChatLineLen;
	CStr			ChatEdit;
	CStrLink		ChannelHead, UserHead;
	CStr			CurrentChannel;

	ClassHash		AITankTeam;
	int32_t				NumTankNames;
	int32_t				NextTankName;	//Since we are shuffling them at load time now.
	CStr			TankNames[MaxTankNames];

	int32_t				NumAvailTeams;
	TeamInfo		Teams[MaxTeams];
//	CStr			OptTeams[MaxTeams];
	int32_t				MaxAllowedTeams;

	int32_t				NumMusicFiles;
	int32_t				NextMusicFile;
	CStr			MusicFiles[MaxMusicFiles];

	int32_t				NumTankTypes;
	TankInfo		TankTypes[MaxTankTypes];

	int32_t				NumMaps;
	MapInfo			Maps[MaxMaps];
	CDediMap		DediMaps[MaxMaps];	//Dedicated server map list.
	int32_t				NumDediMaps;

	ServerEntryEx	ServerHead;
	MasterServer	Master[MAX_MASTERS];
	Network			MasterNet;	//Networking object for communication with the master server.

	VoxelWorld		VW;

	StatsPackage	Stats;
	LadderManager	Ladder;

	int32_t				HeartbeatTime;

	uint32_t	LastMasterPingTime;
	int32_t				FramesOver100MS;

		//Render debugging buffer.
	int32_t				DbgBufLen;
	char			DbgBuf[1024];

private:
	CTankGame();

	void DoHUD();
	void DoInput();
#ifndef HEADLESS
	void DoSfmlEvents();
#endif
	int32_t Announcement(const char *ann1, const char *ann2, EntityGID tank = 0);
	void DoPlayerAndCam();
	CStr RandTankType();	//Chooses a random tank type, taking into account the type limiting variable.

public:
	static CTankGame& Get();

	CStr MMSS(int32_t secs);
	bool DoFrame();
	int32_t Cleanup();
	int32_t EnumerateTanks();
	int32_t EnumerateTeams();

	bool StartGame();
	bool StopGame();
	int32_t IsGameRunning();
	int32_t IsMapDMOnly(int32_t num);

	void ProcessServerCommand(char* sCommand);
	int32_t ReadConfigCfg(const char *name);
	void NewMusic();
	void FindMapTitle(ServerEntryEx *se);
	int32_t Heartbeat(sockaddr_in *dest);
	void SortMaps();
	void LoadTankNames(CStr sFilename);
	void LoadMaps();
	void LoadDediMaps();
	void LoadMusic();
	void ShuffleMusic();

	VoxelWorld*		GetVW() {return &VW;}
	int32_t				GetNextTankName() {return NextTankName;}
	int32_t				GetNumTankNames() {return NumTankNames;}
	int32_t				GetNumTankTypes() {return NumTankTypes;}
	CStr			GetTankName(const int32_t iName) {return TankNames[iName];}
	int32_t				GetNumMaps() {return NumMaps;}
	MapInfo*		GetMap(const int32_t iMap) {return &Maps[iMap];}
	CStr			GetAIPrefix() {return GameSettings.AIPrefix;}
	int32_t				GetNumTeams() {return MIN(MaxAllowedTeams,MIN(NumAvailTeams, GameSettings.NumTeams));}
	int32_t				GetNumAvailTeams() {return NumAvailTeams;}
	int32_t				GetTeamFillSpot();
	TeamInfo		GetTeam(const int32_t iTeam) {return Teams[iTeam];}
	int32_t				TeamIndexFromHash(const int32_t hash, int32_t*index);
	ServerEntryEx*	GetServerHead() {return &ServerHead;}
	Network*		GetMasterNet() {return &MasterNet;}
	CStrLink*		GetUserHead() {return &UserHead;}
	CStrLink*		GetChannelHead() {return &ChannelHead;}
	CStr*			GetChatLine(const int32_t iChat) {return &ChatLine[iChat];}
	int32_t				GetCurChatLine() {return CurChatLine;}
	CStr*			GetCurChatLineText() {return &ChatLine[CurChatLine];}
	int32_t				GetCurChatLineLen() {return ChatLineLen;}
	CStr*			GetChatEdit() {return &ChatEdit;}
	CStr*			GetCurChannel() {return &CurrentChannel;}
	LadderManager*	GetLadder() {return &Ladder;}
	TankInfo		GetTankType(const int32_t iTank) {return TankTypes[iTank];}
	StatsPackage*	GetStats() {return &Stats;}
	SimpleStatus*	GetLoadingStatus() {return &LoadingStatus;}

	CGameState*		GetGameState() {return &GameState;}
	CGameSettings*	GetSettings() {return &GameSettings;}
	CInputState*	GetInputState() {return &InputState;}

	void			SetNextTankName(const int32_t iName) {NextTankName = iName;}
	void			SetCurrentChannel(const char *sChannel) {CurrentChannel = sChannel;}
	void			SetCurChatLine(const int32_t iChat) {CurChatLine = iChat;}
	void			SetCurChatLineText(const char* sChat) {ChatLine[CurChatLine] = sChat;}
};


#ifndef WIN32
#include <unistd.h>
inline void Sleep(unsigned int iMilliseconds)
{
    usleep(iMilliseconds*1000);
}
#endif

#endif

