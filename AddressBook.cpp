#include <string.h>
#include <string>
#include <map>
#include "base64.h"
#include "util.h"
#include "Identity.h"
#include "Log.h"
#include "AddressBook.h"

#include <boost/algorithm/string.hpp>

namespace i2p
{
namespace data
{

AddressBook::AddressBook (): m_IsLoaded (false)
{
}

	
const IdentHash * AddressBook::FindAddress (const std::string& address)
{
	if (!m_IsLoaded)
		LoadHosts ();
	auto it = m_Addresses.find (address);
	if (it != m_Addresses.end ())
		return &it->second;
	else
		return nullptr;	
}

void AddressBook::LoadHostsFromI2P ()
{
	std::string content;
	while (true)
	{
		// TODO: hosts link in config
		int http_code = i2p::util::http::httpRequestViaI2pProxy("http://udhdrtrcetjm5sxzskjyr5ztpeszydbh4dpl3pl4utgqqw2v4jna.b32.i2p/hosts.txt", content);
		if (http_code ==200)
			if (!boost::starts_with(content, "<html>") && !content.empty()) // TODO: test and remove
				break;
		std::this_thread::sleep_for(std::chrono::seconds(5));
	}

	std::ofstream f_save(i2p::util::filesystem::GetFullPath("hosts.txt").c_str(), std::ofstream::out);
	if (f_save.is_open())
	{
		f_save << content;
		f_save.close();
	}
	else
		LogPrint("Can't write hosts.txt");
	m_IsLoaded = false;
	return;
}

void AddressBook::LoadHosts ()
{
	m_IsLoaded = true;
	std::ifstream f (i2p::util::filesystem::GetFullPath ("hosts.txt").c_str (), std::ofstream::in); // in text mode
	if (!f.is_open ())	
	{
		LogPrint ("hosts.txt not found. Try to load...");
		std::thread load_hosts(&AddressBook::LoadHostsFromI2P, this);
		load_hosts.detach();
		return;
	}
	int numAddresses = 0;

	std::string s;

	while (!f.eof ())
	{
		getline(f, s);

		if (!s.length())
			continue; // skip empty line

		size_t pos = s.find('=');

		if (pos != std::string::npos)
		{
			std::string name = s.substr(0, pos++);
			std::string addr = s.substr(pos);

			Identity ident;
			if (!ident.FromBase64(addr)) {
				LogPrint ("hosts.txt: ignore ", name);
				continue;
			}
			m_Addresses[name] = ident.Hash();
			numAddresses++;
		}		
	}
	LogPrint (numAddresses, " addresses loaded");
}

}
}

