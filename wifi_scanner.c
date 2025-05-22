#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <wlanapi.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#pragma comment(lib, "wlanapi.lib")
#pragma comment(lib, "ole32.lib")

typedef struct {
    char ssid[256];
    char bssid[64];
    int signal_strength;
    char security[64];
    int channel;
    int frequency;
    BOOL is_connected;
} WiFiNetwork;

typedef struct {
    WiFiNetwork* networks;
    int count;
    int capacity;
} WiFiNetworkList;

static HANDLE g_hWlanClient = NULL;
static PWLAN_INTERFACE_INFO_LIST g_pInterfaceList = NULL;
static GUID g_interfaceGuid = {0};

void InitializeWiFiScanner(void) {
    DWORD dwResult = 0;
    DWORD dwNegotiatedVersion = 0;
    
    dwResult = WlanOpenHandle(2, NULL, &dwNegotiatedVersion, &g_hWlanClient);
    if (dwResult != ERROR_SUCCESS) {
        g_hWlanClient = NULL;
        return;
    }
    
    dwResult = WlanEnumInterfaces(g_hWlanClient, NULL, &g_pInterfaceList);
    if (dwResult != ERROR_SUCCESS || g_pInterfaceList->dwNumberOfItems == 0) {
        if (g_pInterfaceList != NULL) {
            WlanFreeMemory(g_pInterfaceList);
            g_pInterfaceList = NULL;
        }
        if (g_hWlanClient != NULL) {
            WlanCloseHandle(g_hWlanClient, NULL);
            g_hWlanClient = NULL;
        }
        return;
    }
    
    for (DWORD i = 0; i < g_pInterfaceList->dwNumberOfItems; i++) {
        if (g_pInterfaceList->InterfaceInfo[i].isState == wlan_interface_state_connected ||
            g_pInterfaceList->InterfaceInfo[i].isState == wlan_interface_state_disconnected) {
            g_interfaceGuid = g_pInterfaceList->InterfaceInfo[i].InterfaceGuid;
            break;
        }
    }
}

void CleanupWiFiScanner(void) {
    if (g_pInterfaceList != NULL) {
        WlanFreeMemory(g_pInterfaceList);
        g_pInterfaceList = NULL;
    }
    
    if (g_hWlanClient != NULL) {
        WlanCloseHandle(g_hWlanClient, NULL);
        g_hWlanClient = NULL;
    }
}

void CleanupNetworkList(WiFiNetworkList* list) {
    if (list && list->networks) {
        free(list->networks);
        list->networks = NULL;
        list->count = 0;
        list->capacity = 0;
    }
}

static void AddNetworkToList(WiFiNetworkList* list, const WiFiNetwork* network) {
    if (list->count >= list->capacity) {
        int newCapacity = list->capacity == 0 ? 10 : list->capacity * 2;
        WiFiNetwork* newNetworks = realloc(list->networks, newCapacity * sizeof(WiFiNetwork));
        if (!newNetworks) return;
        
        list->networks = newNetworks;
        list->capacity = newCapacity;
    }
    
    list->networks[list->count] = *network;
    list->count++;
}

static const char* GetSecurityTypeString(DOT11_AUTH_ALGORITHM authAlgo, DOT11_CIPHER_ALGORITHM cipherAlgo) {
    switch (authAlgo) {
        case DOT11_AUTH_ALGO_80211_OPEN:
            if (cipherAlgo == DOT11_CIPHER_ALGO_NONE) {
                return "Open";
            } else {
                return "WEP";
            }
        case DOT11_AUTH_ALGO_80211_SHARED_KEY:
            return "WEP Shared";
        case DOT11_AUTH_ALGO_WPA:
            return "WPA Personal";
        case DOT11_AUTH_ALGO_WPA_PSK:
            return "WPA Personal";
        case DOT11_AUTH_ALGO_WPA_NONE:
            return "WPA (Ad-hoc)";
        case DOT11_AUTH_ALGO_RSNA:
            return "WPA2 Enterprise";
        case DOT11_AUTH_ALGO_RSNA_PSK:
            return "WPA2 Personal";
        case DOT11_AUTH_ALGO_WPA3:
            return "WPA3 Enterprise";
        case DOT11_AUTH_ALGO_WPA3_SAE:
            return "WPA3 Personal";
        case DOT11_AUTH_ALGO_OWE:
            return "Enhanced Open";
        default:
            return "Unknown";
    }
}

static int GetChannelFromFrequency(DWORD frequency) {
    if (frequency >= 2412 && frequency <= 2484) {
        if (frequency == 2484) return 14;
        return (frequency - 2412) / 5 + 1;
    } else if (frequency >= 5170 && frequency <= 5825) {
        return (frequency - 5000) / 5;
    } else if (frequency >= 5955 && frequency <= 7115) {
        return (frequency - 5950) / 5;
    }
    return 0;
}

static BOOL IsNetworkConnected(const GUID* interfaceGuid, const DOT11_SSID* ssid) {
    PWLAN_CONNECTION_ATTRIBUTES pConnectInfo = NULL;
    DWORD connectInfoSize = sizeof(WLAN_CONNECTION_ATTRIBUTES);
    WLAN_OPCODE_VALUE_TYPE opCode = wlan_opcode_value_type_invalid;
    
    DWORD dwResult = WlanQueryInterface(g_hWlanClient, interfaceGuid,
                                       wlan_intf_opcode_current_connection,
                                       NULL, &connectInfoSize,
                                       (PVOID*)&pConnectInfo, &opCode);
    
    if (dwResult != ERROR_SUCCESS || !pConnectInfo) {
        return FALSE;
    }
    
    BOOL isConnected = FALSE;
    if (pConnectInfo->wlanAssociationAttributes.dot11Ssid.uSSIDLength == ssid->uSSIDLength) {
        if (memcmp(pConnectInfo->wlanAssociationAttributes.dot11Ssid.ucSSID,
                  ssid->ucSSID, ssid->uSSIDLength) == 0) {
            isConnected = TRUE;
        }
    }
    
    if (pConnectInfo) {
        WlanFreeMemory(pConnectInfo);
    }
    
    return isConnected;
}

static void ProcessSecurityInfo(WiFiNetwork* network, WLAN_BSS_ENTRY* pBssEntry) {
    BOOL foundSecurity = FALSE;
    
    if (pBssEntry->ulIeSize > 0 && pBssEntry->ulIeOffset > 0) {
        PBYTE pIeData = (PBYTE)pBssEntry + pBssEntry->ulIeOffset;
        DWORD ieOffset = 0;
        
        while (ieOffset + 2 < pBssEntry->ulIeSize) {
            BYTE elementId = pIeData[ieOffset];
            BYTE length = pIeData[ieOffset + 1];
            
            if (ieOffset + 2 + length > pBssEntry->ulIeSize) break;
            
            if (elementId == 48 && length >= 4) {
                strcpy_s(network->security, sizeof(network->security), "WPA2 Personal");
                foundSecurity = TRUE;
                break;
            } else if (elementId == 221 && length >= 8) {
                PBYTE vendorIe = &pIeData[ieOffset + 2];
                if (vendorIe[0] == 0x00 && vendorIe[1] == 0x50 && vendorIe[2] == 0xF2 && vendorIe[3] == 0x01) {
                    strcpy_s(network->security, sizeof(network->security), "WPA Personal");
                    foundSecurity = TRUE;
                    break;
                }
            }
            
            ieOffset += 2 + length;
            if (ieOffset >= pBssEntry->ulIeSize) break;
        }
    }
    
    if (!foundSecurity) {
        if (pBssEntry->usCapabilityInformation & 0x0010) {
            strcpy_s(network->security, sizeof(network->security), "WEP");
        } else {
            strcpy_s(network->security, sizeof(network->security), "Open");
        }
    }
}

static BOOL AddOrUpdateNetwork(WiFiNetworkList* list, const WiFiNetwork* network) {
    for (int i = 0; i < list->count; i++) {
        BOOL isMatch = FALSE;
        
        if (strlen(network->ssid) > 0 && strlen(list->networks[i].ssid) > 0) {
            isMatch = (strcmp(list->networks[i].ssid, network->ssid) == 0);
        } else if (strlen(network->ssid) == 0 && strlen(list->networks[i].ssid) == 0) {
            isMatch = (strcmp(list->networks[i].bssid, network->bssid) == 0);
        }
        
        if (isMatch) {
            if (network->signal_strength > list->networks[i].signal_strength) {
                list->networks[i] = *network;
            }
            return TRUE;
        }
    }
    
    AddNetworkToList(list, network);
    return TRUE;
}

int ScanWiFiNetworks(WiFiNetworkList* list) {
    if (!g_hWlanClient || !list) {
        return -1;
    }
    
    CleanupNetworkList(list);
    
    DWORD dwResult = WlanScan(g_hWlanClient, &g_interfaceGuid, NULL, NULL, NULL);
    if (dwResult != ERROR_SUCCESS) {
        return -1;
    }
    
    PWLAN_BSS_LIST pBssList = NULL;
    for (int attempts = 0; attempts < 10; attempts++) {
        Sleep(200);
        
        dwResult = WlanGetNetworkBssList(g_hWlanClient, &g_interfaceGuid,
                                        NULL, dot11_BSS_type_any, FALSE, NULL, &pBssList);
        
        if (dwResult == ERROR_SUCCESS && pBssList && pBssList->dwNumberOfItems > 0) {
            break;
        }
        
        if (pBssList) {
            WlanFreeMemory(pBssList);
            pBssList = NULL;
        }
        
        MSG msg;
        while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE)) {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
    }
    
    if (dwResult != ERROR_SUCCESS || !pBssList) {
        return -1;
    }
    
    for (DWORD i = 0; i < pBssList->dwNumberOfItems; i++) {
        WLAN_BSS_ENTRY* pBssEntry = &pBssList->wlanBssEntries[i];
        WiFiNetwork network = {0};
        
        if (pBssEntry->dot11Ssid.uSSIDLength > 0) {
            memcpy(network.ssid, pBssEntry->dot11Ssid.ucSSID, 
                   min(pBssEntry->dot11Ssid.uSSIDLength, sizeof(network.ssid) - 1));
            network.ssid[pBssEntry->dot11Ssid.uSSIDLength] = '\0';
        } else {
            strcpy_s(network.ssid, sizeof(network.ssid), "");
        }
        
        sprintf_s(network.bssid, sizeof(network.bssid),
                 "%02X:%02X:%02X:%02X:%02X:%02X",
                 pBssEntry->dot11Bssid[0], pBssEntry->dot11Bssid[1],
                 pBssEntry->dot11Bssid[2], pBssEntry->dot11Bssid[3],
                 pBssEntry->dot11Bssid[4], pBssEntry->dot11Bssid[5]);
        
        network.signal_strength = pBssEntry->lRssi;
        network.frequency = pBssEntry->ulChCenterFrequency / 1000;
        network.channel = GetChannelFromFrequency(network.frequency);
        
        ProcessSecurityInfo(&network, pBssEntry);
        network.is_connected = IsNetworkConnected(&g_interfaceGuid, &pBssEntry->dot11Ssid);
        
        AddOrUpdateNetwork(list, &network);
    }
    
    if (pBssList) {
        WlanFreeMemory(pBssList);
    }
    
    if (list->count > 1) {
        for (int i = 0; i < list->count - 1; i++) {
            for (int j = i + 1; j < list->count; j++) {
                if (list->networks[i].signal_strength < list->networks[j].signal_strength) {
                    WiFiNetwork temp = list->networks[i];
                    list->networks[i] = list->networks[j];
                    list->networks[j] = temp;
                }
            }
        }
    }
    
    return 0;
} 