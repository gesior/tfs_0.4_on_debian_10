////////////////////////////////////////////////////////////////////////
// OpenTibia - an opensource roleplaying game
////////////////////////////////////////////////////////////////////////
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <http://www.gnu.org/licenses/>.
////////////////////////////////////////////////////////////////////////
#include "otpch.h"
#include "otsystem.h"

#include <iostream>
#include <fstream>
#include <iomanip>

#ifndef WINDOWS
#include <unistd.h>
#include <termios.h>
#else
#include <conio.h>
#endif
#include <boost/config.hpp>

#include "server.h"
#ifdef __LOGIN_SERVER__
#include "gameservers.h"
#endif
#include "networkmessage.h"

#include "game.h"
#include "chat.h"
#include "tools.h"
#include "rsa.h"

#include "protocollogin.h"
#include "protocolgame.h"
#include "protocolold.h"
#include "protocolhttp.h"

#include "status.h"
#include "manager.h"
#ifdef __OTADMIN__
#include "admin.h"
#endif

#include "configmanager.h"
#include "scriptmanager.h"
#include "databasemanager.h"

#include "iologindata.h"
#include "ioban.h"

#include "outfit.h"
#include "vocation.h"
#include "group.h"

#include "monsters.h"
#ifdef __OTSERV_ALLOCATOR__
#include "allocator.h"
#endif
#ifdef __EXCEPTION_TRACER__
#include "exception.h"
#endif
#ifndef __OTADMIN__
#include "textlogger.h"
#endif

#ifdef __NO_BOOST_EXCEPTIONS__
#include <exception>

inline void boost::throw_exception(std::exception const & e)
{
	std::clog << "Boost exception: " << e.what() << std::endl;
}
#endif

ConfigManager g_config;
Game g_game;
Monsters g_monsters;
Npcs g_npcs;
RSA g_RSA;
Chat g_chat;

boost::mutex g_loaderLock;
boost::condition_variable g_loaderSignal;
boost::unique_lock<boost::mutex> g_loaderUniqueLock(g_loaderLock);
std::list<std::pair<uint32_t, uint32_t> > serverIps;

bool argumentsHandler(StringVec args)
{
	StringVec tmp;
	for(StringVec::iterator it = args.begin(); it != args.end(); ++it)
	{
		if((*it) == "--help")
		{
			std::clog << "Usage:\n"
			"\n"
			"\t--config=$1\t\tAlternate configuration file path.\n"
			"\t--data-directory=$1\tAlternate data directory path.\n"
			"\t--ip=$1\t\t\tIP address of the server.\n"
			"\t\t\t\tShould be equal to the global IP.\n"
			"\t--login-port=$1\tPort for login server to listen on.\n"
			"\t--game-port=$1\tPort for game server to listen on.\n"
			"\t--admin-port=$1\tPort for admin server to listen on.\n"
			"\t--manager-port=$1\tPort for manager server to listen on.\n"
			"\t--status-port=$1\tPort for status server to listen on.\n";
#ifndef WINDOWS
			std::clog << "\t--runfile=$1\t\tSpecifies run file. Will contain the pid\n"
			"\t\t\t\tof the server process as long as run status.\n";
#endif
			std::clog << "\t--log=$1\t\tWhole standard output will be logged to\n"
			"\t\t\t\tthis file.\n"
			"\t--closed\t\t\tStarts the server as closed.\n";
			return false;
		}

		if((*it) == "--version")
		{
			std::clog << SOFTWARE_NAME << ", version " << SOFTWARE_VERSION << " (" << SOFTWARE_CODENAME << ")\n"
			"Compiled with " << BOOST_COMPILER << " at " << __DATE__ << ", " << __TIME__ << ".\n"
			"A server developed by Elf, Talaturen, KaczooH, Stian and Kornholijo.\n"
			"Visit our forum for updates, support and resources: http://otland.net.\n";
			return false;
		}

		tmp = explodeString((*it), "=");
		if(tmp[0] == "--config")
			g_config.setString(ConfigManager::CONFIG_FILE, tmp[1]);
		else if(tmp[0] == "--data-directory")
			g_config.setString(ConfigManager::DATA_DIRECTORY, tmp[1]);
		else if(tmp[0] == "--ip")
			g_config.setString(ConfigManager::IP, tmp[1]);
		else if(tmp[0] == "--login-port")
			g_config.setNumber(ConfigManager::LOGIN_PORT, atoi(tmp[1].c_str()));
		else if(tmp[0] == "--game-port")
			g_config.setNumber(ConfigManager::GAME_PORT, atoi(tmp[1].c_str()));
		else if(tmp[0] == "--admin-port")
			g_config.setNumber(ConfigManager::ADMIN_PORT, atoi(tmp[1].c_str()));
		else if(tmp[0] == "--manager-port")
			g_config.setNumber(ConfigManager::MANAGER_PORT, atoi(tmp[1].c_str()));
		else if(tmp[0] == "--status-port")
			g_config.setNumber(ConfigManager::STATUS_PORT, atoi(tmp[1].c_str()));
#ifndef WINDOWS
		else if(tmp[0] == "--runfile")
			g_config.setString(ConfigManager::RUNFILE, tmp[1]);
#endif
		else if(tmp[0] == "--log")
			g_config.setString(ConfigManager::OUTPUT_LOG, tmp[1]);
		else if(tmp[0] == "--closed")
			g_config.setBool(ConfigManager::START_CLOSED, true);
		else if(tmp[0] == "--no-script")
			g_config.setBool(ConfigManager::SCRIPT_SYSTEM, false);
	}

	return true;
}

#ifndef WINDOWS
int32_t getch()
{
	struct termios oldt;
	tcgetattr(STDIN_FILENO, &oldt);

	struct termios newt = oldt; 
	newt.c_lflag &= ~(ICANON | ECHO);  
	tcsetattr(STDIN_FILENO, TCSANOW, &newt); 

	int32_t ch = getchar();  
	tcsetattr(STDIN_FILENO, TCSANOW, &oldt); 
	return ch; 
}

void signalHandler(int32_t sig)
{
	switch(sig)
	{
		case SIGHUP:
			Dispatcher::getInstance().addTask(createTask(
				boost::bind(&Game::saveGameState, &g_game, false)));
			break;

		case SIGTRAP:
			g_game.cleanMap();
			break;

		case SIGCHLD:
			g_game.proceduralRefresh();
			break;

		case SIGUSR1:
			Dispatcher::getInstance().addTask(createTask(
				boost::bind(&Game::setGameState, &g_game, GAMESTATE_CLOSED)));
			break;

		case SIGUSR2:
			g_game.setGameState(GAMESTATE_NORMAL);
			break;

		case SIGCONT:
			Dispatcher::getInstance().addTask(createTask(
				boost::bind(&Game::reloadInfo, &g_game, RELOAD_ALL, 0)));
			break;

		case SIGQUIT:
			Dispatcher::getInstance().addTask(createTask(
				boost::bind(&Game::setGameState, &g_game, GAMESTATE_SHUTDOWN)));
			break;

		case SIGTERM:
			Dispatcher::getInstance().addTask(createTask(
				boost::bind(&Game::shutdown, &g_game)));
			break;

		default:
			break;
	}
}

void runfileHandler(void)
{
	std::ofstream runfile(g_config.getString(ConfigManager::RUNFILE).c_str(), std::ios::trunc | std::ios::out);
	runfile.close();
}
#else
int32_t getch()
{
	return (int32_t)getchar();
}
#endif

void allocationHandler()
{
	puts("Allocation failed, server out of memory!\nDecrease size of your map or compile in a 64-bit mode.");
	char buffer[1024];
	delete fgets(buffer, 1024, stdin);
	exit(-1);
}

void startupErrorMessage(std::string error = "")
{
	if(error.length() > 0)
		std::clog << std::endl << "> ERROR: " << error << std::endl;

	getch();
	exit(-1);
}

void otserv(StringVec args, ServiceManager* services);
int main(int argc, char* argv[])
{
	StringVec args = StringVec(argv, argv + argc);
	if(argc > 1 && !argumentsHandler(args))
		return 0;

	std::set_new_handler(allocationHandler);
	ServiceManager servicer;
	g_config.startup();

#ifdef __OTSERV_ALLOCATOR_STATS__
	boost::thread(boost::bind(&allocatorStatsThread, (void*)NULL));
	// TODO: shutdown this thread?
#endif
#ifdef __EXCEPTION_TRACER__
	ExceptionHandler mainExceptionHandler;
	mainExceptionHandler.InstallHandler();
#endif
#ifndef WINDOWS

	// ignore sigpipe...
	struct sigaction sigh;
	sigh.sa_handler = SIG_IGN;
	sigh.sa_flags = 0;

	sigemptyset(&sigh.sa_mask);
	sigaction(SIGPIPE, &sigh, NULL);

	// register signals
	signal(SIGHUP, signalHandler); //save
	signal(SIGTRAP, signalHandler); //clean
	signal(SIGCHLD, signalHandler); //refresh
	signal(SIGUSR1, signalHandler); //close server
	signal(SIGUSR2, signalHandler); //open server
	signal(SIGCONT, signalHandler); //reload all
	signal(SIGQUIT, signalHandler); //save & shutdown
	signal(SIGTERM, signalHandler); //shutdown
#endif

	OutputHandler::getInstance();
	Dispatcher::getInstance().addTask(createTask(boost::bind(otserv, args, &servicer)));

	g_loaderSignal.wait(g_loaderUniqueLock);
	if(servicer.isRunning())
	{
		std::clog << ">> " << g_config.getString(ConfigManager::SERVER_NAME) << " server Online!" << std::endl << std::endl;
		servicer.run();
	}
	else
		std::clog << ">> " << g_config.getString(ConfigManager::SERVER_NAME) << " server Offline! No services available..." << std::endl << std::endl;

#ifdef __EXCEPTION_TRACER__
	mainExceptionHandler.RemoveHandler();
#endif
	return 0;
}

void otserv(StringVec, ServiceManager* services)
{
	srand((uint32_t)OTSYS_TIME());
#if defined(WINDOWS)
	SetConsoleTitle(SOFTWARE_NAME);

#endif
	g_game.setGameState(GAMESTATE_STARTUP);
#if !defined(WINDOWS) && !defined(__ROOT_PERMISSION__)
	if(!getuid() || !geteuid())
	{
		std::clog << "> WARNING: " << SOFTWARE_NAME << " has been executed as super user! It is "
			<< "recommended to run as a normal user." << std::endl << "Continue? (y/N)" << std::endl;
		char buffer = getch();
		if(buffer != 121 && buffer != 89)
			startupErrorMessage("Aborted.");
	}
#endif

	std::clog << SOFTWARE_NAME << ", version " << SOFTWARE_VERSION << " (" << SOFTWARE_CODENAME << ")" << std::endl
		<< "Compiled with " << BOOST_COMPILER << " at " << __DATE__ << ", " << __TIME__ << "." << std::endl
		<< "A server developed by Elf, Talaturen, KaczooH, Stian and Kornholijo." << std::endl
		<< "Visit our forum for updates, support and resources: http://otland.net." << std::endl << std::endl;
	std::stringstream ss;
#ifdef __DEBUG__
	ss << " GLOBAL";
#endif
#ifdef __DEBUG_MOVESYS__
	ss << " MOVESYS";
#endif
#ifdef __DEBUG_CHAT__
	ss << " CHAT";
#endif
#ifdef __DEBUG_EXCEPTION_REPORT__
	ss << " EXCEPTION-REPORT";
#endif
#ifdef __DEBUG_HOUSES__
	ss << " HOUSES";
#endif
#ifdef __DEBUG_LUASCRIPTS__
	ss << " LUA-SCRIPTS";
#endif
#ifdef __DEBUG_MAILBOX__
	ss << " MAILBOX";
#endif
#ifdef __DEBUG_NET__
	ss << " NET";
#endif
#ifdef __DEBUG_NET_DETAIL__
	ss << " NET-DETAIL";
#endif
#ifdef __DEBUG_RAID__
	ss << " RAIDS";
#endif
#ifdef __DEBUG_SCHEDULER__
	ss << " SCHEDULER";
#endif
#ifdef __DEBUG_SPAWN__
	ss << " SPAWNS";
#endif
#ifdef __SQL_QUERY_DEBUG__
	ss << " SQL-QUERIES";
#endif

	std::string debug = ss.str();
	if(!debug.empty())
		std::clog << ">> Debugging:" << debug << "." << std::endl;

	std::clog << ">> Loading config (" << g_config.getString(ConfigManager::CONFIG_FILE) << ")" << std::endl;
	if(!g_config.load())
		startupErrorMessage("Unable to load " + g_config.getString(ConfigManager::CONFIG_FILE) + "!");

	// silently append trailing slash
	std::string path = g_config.getString(ConfigManager::DATA_DIRECTORY);
	g_config.setString(ConfigManager::DATA_DIRECTORY, path.erase(path.find_last_not_of("/") + 1) + "/");

	path = g_config.getString(ConfigManager::LOGS_DIRECTORY);
	g_config.setString(ConfigManager::LOGS_DIRECTORY, path.erase(path.find_last_not_of("/") + 1) + "/");

	std::clog << ">> Opening logs" << std::endl;
	Logger::getInstance()->open();

	IntegerVec cores = vectorAtoi(explodeString(g_config.getString(ConfigManager::CORES_USED), ","));
	if(cores[0] != -1)
	{
#ifdef WINDOWS
		int32_t mask = 0;
		for(IntegerVec::iterator it = cores.begin(); it != cores.end(); ++it)
			mask += 1 << (*it);

		SetProcessAffinityMask(GetCurrentProcess(), mask);
	}

	std::stringstream mutexName;
	mutexName << "forgottenserver_" << g_config.getNumber(ConfigManager::WORLD_ID);

	CreateMutex(NULL, FALSE, mutexName.str().c_str());
	if(GetLastError() == ERROR_ALREADY_EXISTS)
		startupErrorMessage("Another instance of The Forgotten Server is already running with the same worldId.\nIf you want to run multiple servers, please change the worldId in configuration file.");

	std::string defaultPriority = asLowerCaseString(g_config.getString(ConfigManager::DEFAULT_PRIORITY));
	if(defaultPriority == "realtime")
		SetPriorityClass(GetCurrentProcess(), REALTIME_PRIORITY_CLASS);
	else if(defaultPriority == "high")
		SetPriorityClass(GetCurrentProcess(), HIGH_PRIORITY_CLASS);
	else if(defaultPriority == "higher")
		SetPriorityClass(GetCurrentProcess(), ABOVE_NORMAL_PRIORITY_CLASS);

#else
#ifndef MACOS
		cpu_set_t mask;
		CPU_ZERO(&mask);
		for(IntegerVec::iterator it = cores.begin(); it != cores.end(); ++it)
			CPU_SET((*it), &mask);

		sched_setaffinity(getpid(), (int32_t)sizeof(mask), &mask);
	}
#endif

	std::string runPath = g_config.getString(ConfigManager::RUNFILE);
	if(runPath != "" && runPath.length() > 2)
	{
		std::ofstream runFile(runPath.c_str(), std::ios::trunc | std::ios::out);
		runFile << getpid();
		runFile.close();
		atexit(runfileHandler);
	}

	if(!nice(g_config.getNumber(ConfigManager::NICE_LEVEL))) {}
#endif
	std::string encryptionType = asLowerCaseString(g_config.getString(ConfigManager::ENCRYPTION_TYPE));
	if(encryptionType == "md5")
	{
		g_config.setNumber(ConfigManager::ENCRYPTION, ENCRYPTION_MD5);
		std::clog << "> Using MD5 encryption" << std::endl;
	}
	else if(encryptionType == "sha1")
	{
		g_config.setNumber(ConfigManager::ENCRYPTION, ENCRYPTION_SHA1);
		std::clog << "> Using SHA1 encryption" << std::endl;
	}
	else if(encryptionType == "sha256")
	{
		g_config.setNumber(ConfigManager::ENCRYPTION, ENCRYPTION_SHA256);
		std::clog << "> Using SHA256 encryption" << std::endl;
	}
	else if(encryptionType == "sha512")
	{
		g_config.setNumber(ConfigManager::ENCRYPTION, ENCRYPTION_SHA512);
		std::clog << "> Using SHA512 encryption" << std::endl;
	}
	else if(encryptionType == "vahash")
	{
		g_config.setNumber(ConfigManager::ENCRYPTION, ENCRYPTION_VAHASH);
		std::clog << "> Using VAHash encryption" << std::endl;
	}
	else
	{
		g_config.setNumber(ConfigManager::ENCRYPTION, ENCRYPTION_PLAIN);
		std::clog << "> Using plaintext encryption" << std::endl << std::endl
			<< "> WARNING: This method is completely unsafe!" << std::endl
			<< "> Please set encryptionType = \"sha1\" (or any other available method) in config.lua" << std::endl;
		boost::this_thread::sleep(boost::posix_time::seconds(30));
	}

	std::clog << ">> Checking software version...";
	if(xmlDocPtr doc = xmlParseFile(VERSION_CHECK))
	{
		xmlNodePtr p, root = xmlDocGetRootElement(doc);
		if(!xmlStrcmp(root->name, (const xmlChar*)"versions"))
		{
			p = root->children->next;
			if(!xmlStrcmp(p->name, (const xmlChar*)"entry"))
			{
				std::string version;
				int32_t patch, build, timestamp;

				bool tmp = false;
				if(readXMLString(p, "version", version) && version != SOFTWARE_VERSION)
					tmp = true;

				if(readXMLInteger(p, "patch", patch) && patch > VERSION_PATCH)
					tmp = true;

				if(readXMLInteger(p, "build", build) && build > VERSION_BUILD)
					tmp = true;

				if(readXMLInteger(p, "timestamp", timestamp) && timestamp > VERSION_TIMESTAMP)
					tmp = true;

				if(tmp)
				{
					std::clog << " ";
					if(version.find("_SVN") == std::string::npos)
						std::clog << "running sub version, please mind it's unstable and only for testing purposes!";
					else
						std::clog << "outdated, please consider upgrading!";

					std::clog << std::endl << "> Current version information - version: "
						<< SOFTWARE_VERSION << ", patch: " << VERSION_PATCH
						<< ", build: " << VERSION_BUILD << ", timestamp: " << VERSION_TIMESTAMP
						<< "." << std::endl << "> Latest version information - version: "
						<< version << ", patch: " << patch << ", build: " << build
						<< ", timestamp: " << timestamp << "." << std::endl;
					if(g_config.getBool(ConfigManager::CONFIRM_OUTDATED_VERSION) &&
						asLowerCaseString(version).find("_svn") == std::string::npos)
					{
						std::clog << "Continue? (y/N)" << std::endl;
						char buffer = getch();
						if(buffer != 121 && buffer != 89)
							startupErrorMessage("Aborted.");
					}
				}
				else
					std::clog << "up to date!" << std::endl;
			}
			else
				std::clog << "failed checking - malformed entry." << std::endl;
		}
		else
			std::clog << "failed checking - malformed file." << std::endl;

		xmlFreeDoc(doc);
	}
	else
		std::clog << "failed - could not parse remote file (are you connected to any network?)" << std::endl;

	std::clog << ">> Loading RSA key" << std::endl;
	const char* p("14299623962416399520070177382898895550795403345466153217470516082934737582776038882967213386204600674145392845853859217990626450972452084065728686565928113");
	const char* q("7630979195970404721891201847792002125535401292779123937207447574596692788513647179235335529307251350570728407373705564708871762033017096809910315212884101");
	const char* d("46730330223584118622160180015036832148732986808519344675210555262940258739805766860224610646919605860206328024326703361630109888417839241959507572247284807035235569619173792292786907845791904955103601652822519121908367187885509270025388641700821735345222087940578381210879116823013776808975766851829020659073");

	g_RSA.initialize(p, q, d);
	std::clog << ">> Starting SQL connection" << std::endl;

	Database* db = Database::getInstance();
	if(db && db->isConnected())
	{
		std::clog << ">> Running Database Manager" << std::endl;
		if(DatabaseManager::getInstance()->isDatabaseSetup())
		{
			uint32_t version = 0;
			do
			{
				version = DatabaseManager::getInstance()->updateDatabase();
				if(!version)
					break;

				std::clog << "> Database has been updated to version: " << version << "." << std::endl;
			}
			while(version < VERSION_DATABASE);
		}
		else
			startupErrorMessage("The database you have specified in config.lua is empty, please import schemas/<engine>.sql to the database.");

		DatabaseManager::getInstance()->checkTriggers();
		DatabaseManager::getInstance()->checkEncryption();
		if(g_config.getBool(ConfigManager::OPTIMIZE_DATABASE) && !DatabaseManager::getInstance()->optimizeTables())
			std::clog << "> No tables were optimized." << std::endl;
	}
	else
		startupErrorMessage("Couldn't estabilish connection to SQL database!");

	std::clog << ">> Loading items (OTB)" << std::endl;
	if(Item::items.loadFromOtb(getFilePath(FILE_TYPE_OTHER, "items/items.otb")))
		startupErrorMessage("Unable to load items (OTB)!");

	std::clog << ">> Loading items (XML)" << std::endl;
	if(!Item::items.loadFromXml())
	{
		std::clog << "Unable to load items (XML)! Continue? (y/N)" << std::endl;
		char buffer = getch();
		if(buffer != 121 && buffer != 89)
			startupErrorMessage("Unable to load items (XML)!");
	}

	std::clog << ">> Loading groups" << std::endl;
	if(!Groups::getInstance()->loadFromXml())
		startupErrorMessage("Unable to load groups!");

	std::clog << ">> Loading vocations" << std::endl;
	if(!Vocations::getInstance()->loadFromXml())
		startupErrorMessage("Unable to load vocations!");

	std::clog << ">> Loading outfits" << std::endl;
	if(!Outfits::getInstance()->loadFromXml())
		startupErrorMessage("Unable to load outfits!");

	std::clog << ">> Loading chat channels" << std::endl;
	if(!g_chat.loadFromXml())
		startupErrorMessage("Unable to load chat channels!");

	if(g_config.getBool(ConfigManager::SCRIPT_SYSTEM))
	{
		std::clog << ">> Loading script systems" << std::endl;
		if(!ScriptManager::getInstance()->loadSystem())
			startupErrorMessage();
	}
	else
		ScriptManager::getInstance();

	std::clog << ">> Loading mods..." << std::endl;
	if(!ScriptManager::getInstance()->loadMods())
		startupErrorMessage();

	#ifdef __LOGIN_SERVER__
	std::clog << ">> Loading game servers" << std::endl;
	if(!GameServers::getInstance()->loadFromXml(true))
		startupErrorMessage("Unable to load game servers!");

	#endif
	std::clog << ">> Loading experience stages" << std::endl;
	if(!g_game.loadExperienceStages())
		startupErrorMessage("Unable to load experience stages!");

	std::clog << ">> Loading monsters" << std::endl;
	if(!g_monsters.loadFromXml())
	{
		std::clog << "Unable to load monsters! Continue? (y/N)" << std::endl;
		char buffer = getch();
		if(buffer != 121 && buffer != 89)
			startupErrorMessage("Unable to load monsters!");
	}

	std::clog << ">> Loading map and spawns..." << std::endl;
	if(!g_game.loadMap(g_config.getString(ConfigManager::MAP_NAME)))
		startupErrorMessage();

	std::clog << ">> Checking world type... ";
	std::string worldType = asLowerCaseString(g_config.getString(ConfigManager::WORLD_TYPE));
	if(worldType == "open" || worldType == "2" || worldType == "openpvp")
	{
		g_game.setWorldType(WORLDTYPE_OPEN);
		std::clog << "Open PvP" << std::endl;
	}
	else if(worldType == "optional" || worldType == "1" || worldType == "optionalpvp")
	{
		g_game.setWorldType(WORLDTYPE_OPTIONAL);
		std::clog << "Optional PvP" << std::endl;
	}
	else if(worldType == "hardcore" || worldType == "3" || worldType == "hardcorepvp")
	{
		g_game.setWorldType(WORLDTYPE_HARDCORE);
		std::clog << "Hardcore PvP" << std::endl;
	}
	else
	{
		std::clog << std::endl;
		startupErrorMessage("Unknown world type: " + g_config.getString(ConfigManager::WORLD_TYPE));
	}

	std::clog << ">> Initializing game state and binding services..." << std::endl;
	g_game.setGameState(GAMESTATE_INIT);
	IPAddressList ipList;

	std::string ip = g_config.getString(ConfigManager::IP);
	if(asLowerCaseString(ip) == "auto")
	{
		// TODO: automatic shit
	}

	IPAddress m_ip;
	if(ip.size())
	{
		std::clog << "> Global IP address: ";
		uint32_t resolvedIp = inet_addr(ip.c_str());
		if(resolvedIp == INADDR_NONE)
		{
			hostent* host = gethostbyname(ip.c_str());
			if(!host)
			{
				std::clog << "..." << std::endl;
				startupErrorMessage("Cannot resolve " + ip + "!");
			}

			resolvedIp = *(uint32_t*)host->h_addr;
		}

		serverIps.push_front(std::make_pair(resolvedIp, 0));
		m_ip = boost::asio::ip::address_v4(swap_uint32(resolvedIp));

		ipList.push_back(m_ip);
		std::clog << m_ip.to_string() << std::endl;
	}

	ipList.push_back(boost::asio::ip::address_v4(INADDR_LOOPBACK));
	bool owned = false;

	char hostName[128];
	if(!gethostname(hostName, 128))
	{
		if(hostent* host = gethostbyname(hostName))
		{
			std::clog << "> Local IP address(es): ";
			for(uint8_t** addr = (uint8_t**)host->h_addr_list; addr[0]; addr++)
			{
				std::clog << (int32_t)(addr[0][0]) << "." << (int32_t)(addr[0][1]) << "."
					<< (int32_t)(addr[0][2]) << "." << (int32_t)(addr[0][3]) << "\t";

				ipList.push_back(boost::asio::ip::address_v4(*(uint32_t*)(*addr)));
				if(ipList.back() == m_ip)
					owned = true; // fuck yeah

				serverIps.push_front(std::make_pair(*(uint32_t*)(*addr), 0x0000FFFF));
			}

			std::clog << std::endl;
		}
	}

	serverIps.push_front(std::make_pair(LOCALHOST, 0xFFFFFFFF)); // we gotta check it!
	if(ip.size() && !owned)
	{
		ipList.clear();
		ipList.push_back(boost::asio::ip::address_v4(INADDR_ANY));
	}

	services->add<ProtocolStatus>(g_config.getNumber(ConfigManager::STATUS_PORT), ipList);
	services->add<ProtocolManager>(g_config.getNumber(ConfigManager::MANAGER_PORT), ipList);
	#ifdef __OTADMIN__
	services->add<ProtocolAdmin>(g_config.getNumber(ConfigManager::ADMIN_PORT), ipList);
	#endif

	//services->add<ProtocolHTTP>(8080, ipList);
	if(
#ifdef __LOGIN_SERVER__
	true
#else
	!g_config.getBool(ConfigManager::LOGIN_ONLY_LOGINSERVER)
#endif
	)
	{
		services->add<ProtocolLogin>(g_config.getNumber(ConfigManager::LOGIN_PORT), ipList);
		services->add<ProtocolOldLogin>(g_config.getNumber(ConfigManager::LOGIN_PORT), ipList);
	}

	services->add<ProtocolGame>(g_config.getNumber(ConfigManager::GAME_PORT), ipList);
	services->add<ProtocolOldGame>(g_config.getNumber(ConfigManager::LOGIN_PORT), ipList);
	std::clog << "> Bound ports: ";

	std::list<uint16_t> ports = services->getPorts();
	for(std::list<uint16_t>::iterator it = ports.begin(); it != ports.end(); ++it)
		std::clog << (*it) << "\t";

	std::clog << std::endl << ">> Everything smells good, server is starting up..." << std::endl;
	g_game.setGameState(g_config.getBool(ConfigManager::START_CLOSED) ? GAMESTATE_CLOSED : GAMESTATE_NORMAL);
	g_game.start(services);
	g_loaderSignal.notify_all();
}
