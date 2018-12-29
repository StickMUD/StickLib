// UDP-FTP Handling for the Naseftpd.
//
// Adapt and merge this snippet where you need it.

#ifndef _MASTER_C

status check_access(string path, mixed caller, string func, status write);

#endif

#ifndef _CONFIG_H
#include "/include/config.h"
#endif

#define TI this_interactive()

#define	FIND_PLAYER(x)	(object)"/secure/simul_efun"->find_player(x)

#define	ROOTID		"root"
#define	BACKBONEID	"backbone"

#define	DOMAINDIR	"areas"

// FTPD_IP is the IP address of the host running the naseftpd.
#define FTPD_IP "209.83.132.79"

#define	LOG_FTP(x)	{\
	if (file_size("/log/FTP.log") > 100000) {\
		catch(rename("/log/FTP.log.old", "/log/FTP.log"));\
	}\
	write_file("/log/FTP.log", ctime(time()) +" "+ (x));\
}
#define UNIX_PATH	"/Users/sticklib/StickLib/lib%s"
		

nomask void FtpAccess(string a, string b, int c);

static mapping _ftp_users;

void
create_ftp()
{
	_ftp_users = ([ ]);
}

mapping
query_ftp_users()
{
	return copy(_ftp_users);
}

nomask status
ftp_valid_read(string cwd, string name)
{
mixed x;
	sscanf(cwd, UNIX_PATH, cwd);
	if (!(x = _ftp_users[name]))
		return 0;
	if (cwd[<1] != '/' && file_size(cwd) == -2)
		cwd += "/";
	if (check_access(cwd, x, "ftp_read", 0))
		return 1;
	LOG_FTP(sprintf("Invalid read access by %s to %s.\n", name, cwd));
	return 0;
}

nomask status
ftp_valid_write(string cwd, string name)
{
mixed x;
	sscanf(cwd, UNIX_PATH, cwd);
	if (!(x = _ftp_users[name]))
		return 0;
	if (cwd[<1] != '/' && file_size(cwd) == -2)
		cwd += "/";
	if (check_access(cwd, x, "ftp_write", 1))
		return 1;
	LOG_FTP(sprintf("Invalid write access by %s to %s.\n", name, cwd));
	return 0;
}

// With this we'll check if the password was correct...
status
CheckPasswd(string who, string passwd)
{
string tmp, tmp2, real_pw, real_name;
int clevel;
	if (!tmp = read_file(PATH_FOR_PLAYER_SAVE(who)+".o")) {
		LOG_FTP("Unknown user ('"+who+"') tried to open ftp-connection.\n");
		return 0;
	}
	if (sscanf(tmp, "%s\npassword%.1t\"%s\"\n", tmp2, real_pw) < 2) {
		LOG_FTP("User '"+who+"' tried to open ftp-connection, but couldn't find crypted\
 password from the savefile.\n");
		return 0;
	}
	if (crypt(passwd, real_pw) != real_pw) {
		LOG_FTP("User '"+who+"' tried to open ftp-connection, but the password didn't\
 match.\n");
		return 0;
	}
/* Now, we have to include some data for access-info as well: */
	if (sscanf(tmp, "%s\nname%.1t\"%s\"\n", tmp2, real_name) < 2) {
		LOG_FTP("User '"+who+"' tried to open ftp-connection, but couldn't find the\
 real user name from the savefile.\n");
		return 0;
	}
	if (sscanf(tmp, "%s\ncoder_level%.1t%d\n", tmp2, clevel) < 2) {
		LOG_FTP("User '"+who+"' tried to open ftp-connection, but couldn't find the\
 coder level from the savefile.\n");
		return 0;
	}
	_ftp_users[real_name] = ({ real_name, clevel });
	return 1;
}

void
receive_imp(string host, string message, int port)
{
  if (lower_case(message[0..5]) == "nftpd\t")
  {
#if 0
#if __HOST_NAME__==MUDHOST
    if (host != FTPD_IP)
    {
      return;
    }
#endif
#endif
    return FtpAccess(host, message, port);
  }
#if 0
// Hmpfhm. We don't have /secure/inetd anyway... let's just comment it out?
	if (message[0..9]=="udp_query:")
		return udp_query(message[10..],host,port);
	"/secure/inetd"->receive_udp(host, message);
#endif
}

string
_get_path(string file, string user)
{
string p, *path;
int i, j;
	sscanf(file, UNIX_PATH, file);
// Resolve all '..' constructs in file and return the result.
	switch (file[0]) {
	case '/':
		break;
	case '~':
		if (strlen(file) < 2)
			file = sprintf(PATH_USER "%s", user);
		else if (file[1] == '/')
			file = sprintf(PATH_USER "%s/%s", user, file[2..]);
		else file = sprintf(PATH_USER "%s", file[1..]);
	}
	path = explode(file, "/");
	while (i < sizeof(path)) {
	  switch (path[i]) {
		case "": case ".":
			path[i] = 0;
			break;
		case "..":
			path[i] = 0;
			j = i;
			while (--j >= 0) {
			  if (path[j]) {
				path[j] = 0;
				break;
			  }
			}
	  }
	  i++;
	}
	return sprintf("/%s", efun::implode(path, "/"));
}

string
dtime(int when)
{
	return ctime(when);  
#if 0
string month, daytime, foo, day, jahr;
  sscanf(ctime(when),"%s %s %d %s %d",foo, month, day, daytime, jahr);
  return sprintf("%s%s%d. %s %d, %s", foo, (day <= 9 ? ",  " : ", "),
	day, month, jahr, daytime);
#endif
}

string
show_dir(string wo, string user)
{
string ret, curr_file;
mixed *dir;
int isdir,r,w,x, tim, has_w;
object pl;
string date, group, f_user, fname, root_dir;
  
	sscanf(wo, UNIX_PATH, wo);
	tim = time();
	if (wo == "/" || wo == "") {
		dir = get_dir("/*", 7);
		root_dir = 0;
		wo = "";
	} else {
	  if (file_size(wo) < 0)
		dir = get_dir(wo + "/*", 7);
	  else
		dir = get_dir(wo, 7);
	  if (x = sscanf(wo, "/%s/%s", root_dir, fname)) {
		sscanf(fname, "%s/", fname);
	  } else sscanf(wo, "/%s", root_dir);
	  switch (root_dir) {
	case DIR_USER:
		group = "coder"; 
		f_user = fname;
		break;
	case DIR_AREA:
		if (f_user) {
		  group = "domain";
		} else {
		  f_user = "sys";
		  group = "sys";
		}
		break;
	  case "data":
		group = "data";
		break;
	  case "doc":
		group = "doc";
		break;
	  case "open":
	  case "log":
		group = "public";
		break;
	  case "basic":
	  case "bin":
	  case "include":
	  case "lib":
	  case "std":
		group = "mudlib";
	  default:
		group = user = "sys";
	  }
	}
	if (!f_user) f_user = group = "mudlib";
	for (ret = ""; sizeof(dir) > 2; dir = dir[3..]) {
		if (dir[0] == "." || dir[0] == "..")
			continue;
		isdir = (dir[1] == -2);
		curr_file = sprintf("%s/%s", wo, dir[0]);
		if (isdir) {
			w = (ftp_valid_write(curr_file, user)) ? 'w':'-';
			r = (ftp_valid_read(curr_file, user)) ? 'r':'-';
		} else {
			r = 'r';
			if (!has_w) {
			  has_w = ftp_valid_write(curr_file, user) ? 1 : -1;
			}
			w = (has_w < 0) ? '-' : 'w';
		}
		date = dtime(dir[2]);
		if ((tim - dir[2]) < 31536000)
			date = date[4..15];
		else
			date = sprintf("%s  %s", date[4..9], date[<4..]);
		if (!isdir && dir[0][<2..<1]==".c")
			x = find_object(curr_file) ? 'X' : 'x';
		else x = '-';
		if (isdir)
		  ret = sprintf("%sd%c%c%c------   1 %-8s %-10s %8d %s %s\n", ret, r,w,x,
			f_user, group, 512, date,dir[0]);
		else
		  ret = sprintf("%s-%c%c%c------   1 %-8s %-10s %8d %s %s\n",ret, r, w, x,
			f_user, group, dir[1], date, dir[0]);
	}
	if (ret == "")
		return "No files found.\n";
	return ret;
}

nomask void
FtpAccess(string host, string message, int port)
{
string *comp;
string reply, head, tmp, user;
string *dir;
int i;

#if __EFUN_DEFINED__(send_imp)
	comp = explode(message,"\t");
#define FTP_ID   0
#define FTP_SEQ  1
#define FTP_TAG  2
#define FTP_CMD  3
#define FTP_ARG1 4
#define FTP_ARG2 5
#define FTP_ARG3 6

	if (sizeof(comp) <= FTP_CMD || lower_case(comp[FTP_TAG]) != "req") {
		log_file("IMP_MSGS","Host: "+host+":"+port+" - '"+message+"'\n");
		return;
	}

	reply = "INVALID";
	user = lower_case(comp[FTP_ARG1]);
	head = sprintf("%s\t%s\tRPLY\t%s\t", comp[FTP_ID], comp[FTP_SEQ], comp[FTP_CMD]);

	switch (lower_case(comp[FTP_CMD])) {
	case "ping":
		if (sizeof(comp) <= FTP_CMD)
		break;
 		reply = "OK";
		break;

	case "user":
		if (sizeof(comp) <= FTP_ARG1)
			break;
		if (file_size(PATH_FOR_PLAYER_SAVE(user)+".o") < 0) {
			reply = "NONE";
			break;
		}
		reply = sprintf("/u/%s", user);
		break;
    	case "pass":
		if (sizeof(comp) <= FTP_ARG2)
			break;
// Would need to check the password here, btw. *grin*
		if (   CheckPasswd(user,comp[FTP_ARG2]) )
			reply = "OK";
		else
			reply = "FAIL";
		break;
    
	case "resv":
		if (sizeof(comp) <= FTP_ARG3)
			break;
		if ( comp[FTP_ARG2] == "" ||
		-1 == member("+~/", comp[FTP_ARG2][0]))
			comp[FTP_ARG2] = sprintf("%s/%s", comp[FTP_ARG3], comp[FTP_ARG2]);
		reply = _get_path(comp[FTP_ARG2], user);
		break;
    
	case "read":
		if (sizeof(comp) <= FTP_ARG2)
			break;
		if (ftp_valid_read(comp[FTP_ARG2], user))
// && comp[FTP_ARG2][0] == '/'
			reply = "OK";
		else
			reply = "FAIL";
		break;
    
	case "writ":
		if (sizeof(comp) <= FTP_ARG2)
			break;
		if (ftp_valid_write(comp[FTP_ARG2], user))
// && comp[FTP_ARG2][0] == '/'
			reply = "OK";
		else
			reply = "FAIL";
		break;

	case "list":
		if (sizeof(comp) <= FTP_ARG2)
			break;
		dir = explode(show_dir(comp[FTP_ARG2], user),"\n");
		for (i = 0; i < sizeof(dir); i++)
			send_imp(host, port, head+"LINE\t"+dir[i]+"\r\n");
		reply = "OK";
		break;

	default:
		log_file("IMP_MSGS","Host: "+host+":"+port+" - '"+message+"'\n");
		break;
	}
	send_imp(host, port, head+reply);
#endif
}
