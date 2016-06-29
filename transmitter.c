/*
	We Kill People™
	R-21 Limited.
*/

#include <Windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <IPHlpApi.h>
#include <WinDNS.h>
#include <wlanapi.h>
#include <objbase.h>
#include <wtypes.h>

// https://api.ipify.org/

#pragma comment(lib , "iphlpapi.lib") //For iphlpapi
#pragma comment(lib , "ws2_32.lib") //For winsock
#pragma comment(lib , "Dnsapi.lib") //For dnsapi
#pragma comment(lib, "wlanapi.lib")
#pragma comment(lib, "ole32.lib")


char* getip()
{

	WSADATA wsa;
	SOCKET s;
	struct sockaddr_in server;
	char *message, server_reply[2000];
	int recv_size;

	if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0)
	{
		return "" ;
	}


	if ((s = socket(AF_INET, SOCK_STREAM, 0)) == INVALID_SOCKET)
	{
		return "";
	}


	struct hostent *host;
	host = gethostbyname("api.ipify.org");
	server.sin_addr.s_addr = *((unsigned long*)host->h_addr);
	server.sin_family = AF_INET;
	server.sin_port = htons(80);

	if (connect(s, (struct sockaddr *)&server, sizeof(server)) < 0)
	{
		return "";
	}


	message = "GET / HTTP/1.1\r\nHost: api.ipify.org\r\nConnection: close\r\n\r\n";
	if (send(s, message, strlen(message), 0) < 0)
	{
		return "";
	}

	if ((recv_size = recv(s, server_reply, 2000, 0)) == SOCKET_ERROR)
	{
		return "";
	}


	server_reply[recv_size] = '\0';


	char * buffer = (char*)malloc(sizeof(char) * strlen(server_reply));
	
	strncpy_s(buffer, MAX_PATH, server_reply, MAX_PATH);

	closesocket(s);
	WSACleanup();
	return buffer;

}

char* get_wlan()
{
	HANDLE hClient = NULL;
	DWORD dwMaxClient = 2;       
	DWORD dwCurVersion = 0;
	DWORD dwResult = 0;
	DWORD dwRetVal = 0;
	int iRet = 0;

	char * wlanname = (char*)malloc(MAX_PATH * sizeof(char));

	WCHAR GuidString[39] = { 0 };

	unsigned int i, j;

	PWLAN_INTERFACE_INFO_LIST pIfList = NULL;
	PWLAN_INTERFACE_INFO pIfInfo = NULL;

	PWLAN_AVAILABLE_NETWORK_LIST pBssList = NULL;
	PWLAN_AVAILABLE_NETWORK pBssEntry = NULL;

	int iRSSI = 0;

	dwResult = WlanOpenHandle(dwMaxClient, NULL, &dwCurVersion, &hClient);
	if (dwResult != ERROR_SUCCESS) {
		return "";
	}

	dwResult = WlanEnumInterfaces(hClient, NULL, &pIfList);
	if (dwResult != ERROR_SUCCESS) {
		return "";
	}

	else
	{
		for (i = 0; i < (int)pIfList->dwNumberOfItems; i++) {

			pIfInfo = (WLAN_INTERFACE_INFO *)&pIfList->InterfaceInfo[i];
			dwResult = WlanGetAvailableNetworkList(hClient,
				&pIfInfo->InterfaceGuid,
				0,
				NULL,
				&pBssList);

			if (dwResult != ERROR_SUCCESS) {

				dwRetVal = 1;
			}
			else {


				for (j = 0; j < pBssList->dwNumberOfItems; j++) {
					pBssEntry =
						(WLAN_AVAILABLE_NETWORK *)& pBssList->Network[j];

					if (pBssEntry->dwFlags) {

						if (pBssEntry->dwFlags & WLAN_AVAILABLE_NETWORK_CONNECTED)
						{
							sprintf_s(wlanname, MAX_PATH, "%s", pBssEntry->dot11Ssid.ucSSID);

						}
					}
				}
			}
		}
	}

	return wlanname;
}

void getmacaddr(unsigned char *mac, struct in_addr destip)
{
	DWORD ret;
	IPAddr srcip;
	ULONG MacAddr[2];
	ULONG PhyAddrLen = 6;  /* default to length of six bytes */
	int i;

	srcip = 0;

	//Send an arp packet
	ret = SendARP((IPAddr)destip.S_un.S_addr, srcip, MacAddr, &PhyAddrLen);

	//Prepare the mac address
	if (PhyAddrLen)
	{
		BYTE *bMacAddr = (BYTE *)& MacAddr;
		for (i = 0; i < (int)PhyAddrLen; i++)
		{
			mac[i] = (char)bMacAddr[i];
		}
	}
}

char *get_mac()
{
	unsigned char mac[6];
	struct in_addr srcip;
	char * proto = "%.2X-%.2X-%.2X-%.2X-%.2X-%.2X";
	char *buffer;
	WSADATA firstsock;

	if (WSAStartup(MAKEWORD(2, 2), &firstsock) != 0)
	{
		return "";
	}


	// get local ip
	char szHostName[256];
	PDNS_RECORD pDnsRecord;
	gethostname(szHostName, 256);
	DNS_STATUS statsus = DnsQuery(szHostName, DNS_TYPE_A, DNS_QUERY_STANDARD, NULL, &pDnsRecord, NULL);
	IN_ADDR ipaddr;
	ipaddr.S_un.S_addr = (pDnsRecord->Data.A.IpAddress);


	srcip.s_addr = inet_addr(inet_ntoa(ipaddr));
	// get mac address
	getmacaddr(mac, srcip);

	// get wlan name

	buffer = (char*)malloc(sizeof(char) * MAX_PATH);

	sprintf_s(buffer, MAX_PATH, proto, mac[0], mac[1], mac[2], mac[3], mac[4], mac[5], inet_ntoa(ipaddr));

	return buffer;
}

int core()
{
	char* macaddr = (char*)malloc(sizeof(char*) * MAX_PATH);
	char * wlanname = (char*)malloc(sizeof(char*) * MAX_PATH);
	char * ipaddr = (char*)malloc(sizeof(char*) * MAX_PATH);
	macaddr = get_mac();
	wlanname = get_wlan();
	ipaddr = getip();

	char * buffer = (char*)malloc(strlen(macaddr)* strlen(wlanname) *strlen(ipaddr));
	sprintf_s(buffer, strlen(macaddr)* strlen(wlanname) *strlen(ipaddr), "Device Mac: %s\nWlan Name: %s\nHTTP RESPONSE: %s", macaddr, wlanname, ipaddr);
	
	free(macaddr);
	free(wlanname);
	free(ipaddr);
	

	// send buffer
	MessageBoxA(NULL, buffer, "", MB_OK);


	free(buffer);
	return 0;
}

int CALLBACK WinMain(
	_In_ HINSTANCE hInstance,
	_In_ HINSTANCE hPrevInstance,
	_In_ LPSTR     lpCmdLine,
	_In_ int       nCmdShow
)
{
	
	return core();
}
