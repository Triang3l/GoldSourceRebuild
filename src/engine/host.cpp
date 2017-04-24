#include <ctime>

#include "quakedef.h"
#include "buildnum.h"
#include "cdaudio.h"
#include "cdll_int.h"
#include "chase.h"
#include "cl_parsefn.h"
#include "client.h"
#include "cmodel.h"
#include "decals.h"
#include "delta.h"
#include "DemoPlayerWrapper.h"
#include "hashpak.h"
#include "net_chan.h"
#include "pmove.h"
#include "qgl.h"
#include "server.h"
#include "snd.h"
#include "sv_main.h"
#include "SystemWrapper.h"
#include "vgui_int.h"
#include "view.h"
#include "voice.h"
#include "wad.h"

quakeparms_t host_parms = {};

bool host_initialized = false;
double realtime = 0;

cvar_t console = { "console", "0.0", FCVAR_ARCHIVE };

unsigned short* host_basepal = nullptr;

int host_hunklevel = 0;

void Host_InitCommands();

void Host_InitLocal()
{
	Host_InitCommands();
	//TODO: implement - Solokiller
	/*
	Cvar_RegisterVariable( &host_killtime );
	Cvar_RegisterVariable( &sys_ticrate );
	Cvar_RegisterVariable( &fps_max );
	Cvar_RegisterVariable( &fps_override );
	Cvar_RegisterVariable( &host_name );
	Cvar_RegisterVariable( &host_limitlocal );
	sys_timescale.value = 1;
	Cvar_RegisterVariable( &host_framerate );
	Cvar_RegisterVariable( &host_speeds );
	Cvar_RegisterVariable( &host_profile );
	Cvar_RegisterVariable( &mp_logfile );
	Cvar_RegisterVariable( &mp_logecho );
	Cvar_RegisterVariable( &sv_log_onefile );
	Cvar_RegisterVariable( &sv_log_singleplayer );
	Cvar_RegisterVariable( &sv_logsecret );
	Cvar_RegisterVariable( &sv_stats );
	Cvar_RegisterVariable( &developer );
	Cvar_RegisterVariable( &deathmatch );
	Cvar_RegisterVariable( &coop );
	Cvar_RegisterVariable( &pausable );
	Cvar_RegisterVariable( &skill );
	*/

	SV_SetMaxclients();
}

void Host_Error( const char* error, ... )
{
	static bool inerror = false;

	char string[ 1024 ];
	va_list va;

	va_start( va, error );

	if( inerror == false )
	{
		inerror = true;

		//TODO: implement - Solokiller
		//SCR_EndLoadingPlaque();

		vsnprintf( string, ARRAYSIZE( string ), error, va );

		//TODO: implement - Solokiller
		/*
		if( !( _DWORD ) sv.active && developer.value != 0.0 )
			CL_WriteMessageHistory( 0, 0 );
			*/

		Con_Printf( "Host_Error: %s\n", string );

		//TODO: implement - Solokiller
		/*
		if( ( _DWORD ) sv.active )
			Host_ShutdownServer( 0 );

		if( ( _DWORD ) cls.state )
		{
			CL_Disconnect();
			cls.demonum = -1;
			inerror = false;
			longjmp( host_abortserver, 1 );
		}
		*/

		Sys_Error( "Host_Error: %s\n", string );
	}

	va_end( va );

	Sys_Error( "Host_Error: recursively entered" );
}

void CheckGore()
{
	char szBuffer[ 128 ];

	Q_memset( szBuffer, 0, sizeof( szBuffer ) );

	if( bLowViolenceBuild )
	{
		Cvar_SetValue( "violence_hblood", 0 );
		Cvar_SetValue( "violence_hgibs", 0 );
		Cvar_SetValue( "violence_ablood", 0 );
		Cvar_SetValue( "violence_agibs", 0 );
	}
	else
	{
		Cvar_SetValue( "violence_hblood", 1 );
		Cvar_SetValue( "violence_hgibs", 1 );
		Cvar_SetValue( "violence_ablood", 1 );
		Cvar_SetValue( "violence_agibs", 1 );
	}
}

void Host_Version()
{
	Q_strcpy( gpszVersionString, "1.0.1.4" );
	Q_strcpy( gpszProductString, "valve" );

	char szFileName[ FILENAME_MAX ];

	strcpy( szFileName, "steam.inf" );

	FileHandle_t hFile = FS_Open( szFileName, "r" );

	if( hFile != FILESYSTEM_INVALID_HANDLE )
	{
		const int iSize = FS_Size( hFile );
		void* pFileData = Mem_Malloc( iSize + 1 );
		FS_Read( pFileData, iSize, hFile );
		FS_Close( hFile );

		char* pBuffer = reinterpret_cast<char*>( pFileData );

		pBuffer[ iSize ] = '\0';

		const int iProductNameLength = Q_strlen( "ProductName=" );
		const int iPatchVerLength = Q_strlen( "PatchVersion=" );

		char szSteamVersionId[ 32 ];

		//Parse out the version and name.
		for( int i = 0; ( pBuffer = COM_Parse( pBuffer ) ) != nullptr && *com_token && i < 2; )
		{
			if( !Q_strnicmp( com_token, "PatchVersion=", iPatchVerLength ) )
			{
				++i;

				Q_strncpy( gpszVersionString, &com_token[ iPatchVerLength ], ARRAYSIZE( gpszVersionString ) );

				if( COM_CheckParm( "-steam" ) )
				{
					FS_GetInterfaceVersion( szSteamVersionId, ARRAYSIZE( szSteamVersionId ) - 1 );
					snprintf( gpszVersionString, ARRAYSIZE( gpszVersionString ), "%s/%s", &com_token[ iPatchVerLength ], szSteamVersionId );
				}
			}
			else if( !Q_strnicmp( com_token, "ProductName=", iProductNameLength ) )
			{
				++i;
				Q_strncpy( gpszProductString, &com_token[ iProductNameLength ], ARRAYSIZE( gpszProductString ) );
			}
		}

		if( pFileData )
			Mem_Free( pFileData );
	}

	//TODO: implement - Solokiller
	/*
	if( ( _DWORD ) cls.state )
	{
		Con_DPrintf( "Protocol version %i\nExe version %s (%s)\n", PROTOCOL_VERSION, gpszVersionString, gpszProductString );
		Con_DPrintf( "Exe build: " __TIME__ " " __DATE__ " (%i)\n", build_number() );
	}
	else
	{
		Con_Printf( "Protocol version %i\nExe version %s (%s)\n", PROTOCOL_VERSION, gpszVersionString, gpszProductString );
		Con_Printf( "Exe build: " __TIME__ " " __DATE__ " (%i)\n", build_number() );
	}
	*/
}

bool Host_Init( quakeparms_t* parms )
{
	srand( time( nullptr ) );

	host_parms = *parms;

	realtime = 0;

	com_argc = parms->argc;
	com_argv = parms->argv;

	Memory_Init( parms->membase, parms->memsize );

	Voice_RegisterCvars();
	Cvar_RegisterVariable( &console );

	if( COM_CheckParm( "-console" ) || COM_CheckParm( "-toconsole" ) || COM_CheckParm( "-dev" ) )
		Cvar_DirectSet( &console, "1.0" );

	Host_InitLocal();

	if( COM_CheckParm( "-dev" ) )
		Cvar_SetValue( "developer", 1.0 );

	Cbuf_Init();
	Cmd_Init();
	Cvar_Init();
	Cvar_CmdInit();

	V_Init();
	Chase_Init();

	COM_Init();
	Host_ClearSaveDirectory();
	HPAK_Init();

	W_LoadWadFile( "gfx.wad" );
	W_LoadWadFile( "fonts.wad" );

	Key_Init();
	Con_Init();
	Decal_Init();
	Mod_Init();
	NET_Init();
	Netchan_Init();
	DELTA_Init();
	SV_Init();
	SystemWrapper_Init();
	Host_Version();

	char versionString[ 256 ];
	snprintf( versionString, ARRAYSIZE( versionString ), "%s,%i,%i", gpszVersionString, PROTOCOL_VERSION, build_number() );

	Cvar_Set( "sv_version", versionString );

	Con_DPrintf( "%4.1f Mb heap\n", parms->memsize / (1024 * 1024.0 ) );

	R_InitTextures();
	HPAK_CheckIntegrity( "custom" );

	Q_memset( &g_module, 0, sizeof( g_module ) );

	if( cls.state != ca_dedicated )
	{
		byte* pPalette = COM_LoadHunkFile( "gfx/palette.lmp" );

		if( !pPalette )
			Sys_Error( "Host_Init: Couldn't load gfx/palette.lmp" );

		byte* pSource = pPalette;

		//Convert the palette from BGR to RGBA. TODO: these are the right formats, right? - Solokiller
		host_basepal = reinterpret_cast<unsigned short*>( Hunk_AllocName( 4 * 256 * sizeof( unsigned short ), "palette.lmp" ) );

		for( int i = 0; i < 256; ++i, pSource += 3 )
		{
			host_basepal[ ( 4 * i ) ] = *( pSource + 2 );
			host_basepal[ ( 4 * i ) + 1 ] = *( pSource + 1 );
			host_basepal[ ( 4 * i ) + 2 ] = *pSource;
			host_basepal[ ( 4 * i ) + 3 ] = 0;
		}

		GL_Init();
		PM_Init( &g_clmove );
		CL_InitEventSystem();
		ClientDLL_Init();
		VGui_Startup();

		if( !VID_Init( host_basepal ) )
		{
			VGui_Shutdown();
			return false;
		}

		Draw_Init();
		SCR_Init();
		R_Init();
		S_Init();
		CDAudio_Init();
		Voice_Init( "voice_speex", 1 );
		DemoPlayer_Init();
		CL_Init();
	}
	else
	{
		Cvar_RegisterVariable( &suitvolume );
	}

	Cbuf_InsertText( "exec valve.rc\n" );

	if( cls.state != ca_dedicated )
		GL_Config();

	Hunk_AllocName( 0, "-HOST_HUNKLEVEL-" );
	host_hunklevel = Hunk_LowMark();
	//TODO: define constants - Solokiller
	giActive = 1;
	scr_skipupdate = false;

	CheckGore();

	host_initialized = true;

	return true;
}

void Host_Shutdown()
{
	static bool isdown = false;

	if( isdown )
	{
		puts( "recursive shutdown" );
	}
	else
	{
		isdown = true;

		//TODO: implement - Solokiller
		/*
		if( host_initialized )
			Host_WriteConfiguration();
			*/

		//TODO: implement - Solokiller
		/*
		SV_ServerShutdown();
		Voice_Deinit();
		*/

		host_initialized = false;

		//TODO: implement - Solokiller
		/*
		CDAudio_Shutdown();
		*/
		VGui_Shutdown();

		//TODO: implement - Solokiller
		/*
		if( ( _DWORD ) cls.state )
			ClientDLL_Shutdown();
			*/

		Cmd_RemoveGameCmds();
		Cmd_Shutdown();
		Cvar_Shutdown();

		//TODO: implement - Solokiller
		/*
		HPAK_FlushHostQueue();
		SV_DeallocateDynamicData();
		*/

		for( int i = 0; i < svs.maxclientslimit; ++i )
		{
			SV_ClearFrames( &svs.clients[ i ].frames );
		}

		//TODO: implement - Solokiller
		/*
		SV_Shutdown();
		SystemWrapper_ShutDown();

		NET_Shutdown();
		S_Shutdown();
		Con_Shutdown();
		*/

		ReleaseEntityDlls();

		//TODO: implement - Solokiller
		/*
		CL_ShutDownClientStatic();
		CM_FreePAS();

		if( wadpath )
		{
			Mem_Free( wadpath );
			wadpath = nullptr;
		}

		if( ( _DWORD ) cls.state )
			Draw_Shutdown();

		Draw_DecalShutdown();

		W_Shutdown();
		*/

		Log_Printf( "Server shutdown\n" );
		//TODO: implement - Solokiller
		/*
		Log_Close();

		COM_Shutdown();
		CL_Shutdown();
		DELTA_Shutdown();
		*/

		Key_Shutdown();

		realtime = 0;

		//TODO: implement - Solokiller
		//sv.time = 0;
		//cl.time = 0;
	}
}

void Host_InitCommands()
{
	//TODO: implement - Solokiller
}
