#include "version_server.h"




bool Version_Server::is_version_server()
{
    std::ifstream fconf;
    std::string conf;
    std::string tmp = NETCONFIG;

    if (access(tmp.c_str(), 0) < 0)
    { 
        error("Invalid file...  Please check if the configuration file exists!");
        return "";
    }
    fconf.open(tmp.c_str());
    nlohmann::json m_Json;
    fconf >> m_Json;
    fconf.close();

    return m_Json["cond"]["is_version_server"].get<bool>();
}

string Version_Server::GetAbsopath()
{
    int i;
    char buf[1024];
    int len = readlink("/proc/self/exe", buf, 1024 - 1);

    if (len < 0 || (len >= 1024 - 1))
    {
        return "";
    }

    buf[len] = '\0';
    for (i = len; i >= 0; i--)
    {
        if (buf[i] == '/')
        {
            buf[i + 1] = '\0';
            break;
        }
    }
    return string(buf);
}

bool Version_Server::init(const string &filename)
{
    ifstream fconf;
    string conf;
    string tmp = this->GetAbsopath();
    tmp += filename;

    if (access(tmp.c_str(), 0) < 0)
    {
        if (false == NewFile(tmp) )
        {
            error("new file failure...  Please check if the configuration file exists!");
            return false;
        }
        debug("please config version and url info");
        return false;
    }

    fconf.open(tmp.c_str());
    if (!fconf.is_open())
    {   
        error("open file failure...  Please check if the configuration file exists!");
        return false;
    }
    fconf >> m_Json;
    fconf.close();

    m_latest_version = m_Json["latest_version"].get<string>();
    m_url = m_Json["url"].get<string>();
    return true;
}

bool Version_Server::write_file(const std::string &filename)
{
    std::ofstream fconf;
    std::string fileName ;

    fileName = this->GetAbsopath() + filename;

    fconf.open( fileName.c_str() );
    fconf << m_Json.dump(4);
    fconf.close();
    return true;
}

bool Version_Server::NewFile(const string &strFile)
{
    
    ofstream file( strFile.c_str(), fstream::out );
    if( file.fail() )
    {
        debug(" file_path = %s", strFile.c_str());
		return false;
    }
	
    file << \
"{ \
    \"latest_version\":\"\",\n \
     \"url\":\"\"\n\
}";
    file.close();

    return true;
}

bool Version_Server::is_update(const node_version_info &version)
{   
    m_Json[version.mac][version.local_ip] = version.version;
    if(version.version == m_latest_version)
    {   
        debug("the version has the latest version.");
        return false;
    }
    else
    {   
        debug("the version need to update...");
        return true;
    }    
}


bool Version_Server::update_configFile(const node_version_info &version)
{
    m_Json[version.mac][version.local_ip] = version.version;
    bool bl = write_file();
    if(bl)
    {
        debug("configFile of versin has been updated.");
        return true;
    }
    else
    {
        debug("configFile of version undate failure...");
        return false;
    }  
}


string Version_Server::get_latest_version_info()
{
    latest_version_info info;
    info.version = m_latest_version;
    info.url = m_url;
    info.hash = getsha256hash(m_latest_version + m_url);

    cout << "最新版本号：" << info.version << endl;
    cout << "url：" << info.url << endl;
    cout << "hash:" << info.hash << endl;

    string tmp = info.version + " ";
    tmp += info.url;
    tmp += " ";
    tmp += info.hash;
    tmp += " ";

    return tmp;
}

