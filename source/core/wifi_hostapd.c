#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <unistd.h>

//#include <wifi_hal_priv.h>
#include "wifi_util.h"


#define MAX_BSS 16
#define MAX_SSID_LEN 32

#define MACF_TO_MAC(macstr, mac)                                                           \
    sscanf(macstr, "%02hhx:%02hhx:%02hhx:%02hhx:%02hhx:%02hhx", &mac[0], &mac[1], &mac[2], \
        &mac[3], &mac[4], &mac[5])

int g_idx = 0;

static const radio_interface_mapping_t *l_radio_interface_map;
static unsigned int l_radio_interface_map_size;
static const radio_interface_mapping_t static_radio_interface_map[] = {
    { 0, 0, "radio1", "wl0"},
    { 1, 1, "radio2", "wl1"},
};

static wifi_interface_name_idex_map_t *interface_index_map = NULL;
static unsigned int interface_index_map_size;
static wifi_interface_name_idex_map_t static_interface_index_map[] = {
	{0, 0,  "wl0.1",   "",  "brlan0",  100,    0,      "private_ssid_2g"},
	{1, 1,  "wl1.1",   "",  "brlan0",  100,    1,      "private_ssid_5g"},
	{0, 0,  "wl0.2",   "",  "brlan9",  101,    2,      "iot_ssid_2g"},
	{1, 1,  "wl1.2",   "",  "brlan10", 101,    3,      "iot_ssid_5g"},
	{0, 0,  "wl0.7",   "",  "brlan6",    0,    12,     "mesh_backhaul_2g"},
	{1, 1,  "wl1.7",   "",  "brlan7",    0,    13,     "mesh_backhaul_5g"},
	{0, 0,  "wl0",     "",  "",          0,    14,     "mesh_sta_2g"},
	{1, 1,  "wl1",     "",  "",          0,    15,     "mesh_sta_5g"},
};

static const char *vap_ifname_map[] = {
    "wl0",   // vapIndex 0
    "wl1",   // vapIndex 1
    "wl0.1", // vapIndex 2
    "wl1.1", // vapIndex 3
    "wl0.2", // vapIndex 4
    "wl1.2", // vapIndex 5
    "wl0.7", // vapIndex 14
    "wl1.7"  // vapIndex 15
};


typedef struct {
    int enable;
    char bss[8];
    char bssid[32];
    char ssid[MAX_SSID_LEN];
    int num_sta;
} wifi_bss_t;

wifi_bss_t bss_info[MAX_BSS];

static const char* get_ifname_from_vap_index(int vapIndex)
{
    if (vapIndex < 0 || vapIndex > 15)
        return NULL;
    return vap_ifname_map[vapIndex];
}

static inline void init_static_interface_map(void)
{
    interface_index_map = static_interface_index_map;
    interface_index_map_size = (sizeof(static_interface_index_map) /
        sizeof(*static_interface_index_map));

    l_radio_interface_map = static_radio_interface_map;
    l_radio_interface_map_size = (sizeof(static_radio_interface_map) /
        sizeof(*static_radio_interface_map));
}

static void init_interface_map(void)
{
    unsigned int i;
    int json_ret;

    //json_ret = init_json_interface_map();
    json_ret = -1;
    if (json_ret < 0) {
        init_static_interface_map();
    }
    wifi_util_info_print(WIFI_CTRL, "%s:%d: Using %s Interface Map\n", __func__, __LINE__,
        ((json_ret < 0) ? "STATIC" : "JSON"));
    wifi_util_info_print(WIFI_CTRL, "%s:%d: Interface Index Map(%u):\n", __func__, __LINE__,
        interface_index_map_size);
    for (i = 0; i < interface_index_map_size; i++) {
        wifi_util_info_print(WIFI_CTRL,
            "\t[%u]={phy_index:%u, rdk_radio_index:%u, interface_name:%s, "
            "mld_interface_name:%s, bridge_name:%s, vlan_id:%d, index:%u, vap_name:%s}\n",
            i, interface_index_map[i].phy_index, interface_index_map[i].rdk_radio_index,
            interface_index_map[i].interface_name, interface_index_map[i].mld_interface_name,
            interface_index_map[i].bridge_name, interface_index_map[i].vlan_id,
            interface_index_map[i].index, interface_index_map[i].vap_name);
    }

    wifi_util_info_print(WIFI_CTRL, "%s:%d: Radio Interface Index Map(%u):\n", __func__, __LINE__,
        l_radio_interface_map_size);
    for (i = 0; i < l_radio_interface_map_size; i++) {
        wifi_util_info_print(WIFI_CTRL, "\t[%u]={phy_index:%u, radio_index:%u, radio_name:%s, "
                            "interface_name:%s}\n",
            i, l_radio_interface_map[i].phy_index, l_radio_interface_map[i].radio_index,
            l_radio_interface_map[i].radio_name, l_radio_interface_map[i].interface_name);
    }
}

#if 0
static int get_wifi_interface_name_from_vap_index(unsigned int vap_index, char *interface_name)
{
    // OneWifi interafce mapping with vap_index
    unsigned char l_index = 0;
    unsigned char total_num_of_vaps = 0;
    const char *l_interface_name = NULL;
    wifi_radio_info_t *radio;
    for (l_index = 0; l_index < g_wifi_hal.num_radios; l_index++) {
        radio = &g_wifi_hal.radio_info[l_index];
        total_num_of_vaps += radio->capab.maxNumberVAPs;
    }
    wifi_util_error_print(WIFI_CTRL, "%s:%d: total_num_of_vaps:%d radio_num:%d\n", __func__, __LINE__,
        total_num_of_vaps, g_wifi_hal.num_radios);
    if ((vap_index >= total_num_of_vaps) || (interface_name == NULL)) {
        wifi_hal_error_print(WIFI_CTRL, "%s:%d: Wrong vap_index:%d \n", __func__, __LINE__, vap_index);
        return RETURN_ERR;
    } else {
        wifi_util_dbg_print(WIFI_CTRL, "%s:%d:  vap_index:%d \n", __func__, __LINE__, vap_index);
    }

    for (l_index = 0; l_index < get_sizeof_interfaces_index_map(); l_index++) {
        if (interface_index_map[l_index].index == vap_index) {
            l_interface_name = interface_index_map[l_index].interface_name;
            strncpy(interface_name, l_interface_name, (strlen(l_interface_name) + 1));
            wifi_hal_dbg_print(WIFI_CTRL, "%s:%d: VAP index %d: interface name %s\n", __func__, __LINE__,
                vap_index, interface_name);
            return RETURN_OK;
        }
    }

    wifi_util_error_print(WIFI_CTRL, "%s:%d: Interface name not found:%d \n", __func__, __LINE__, vap_index);

    return RETURN_ERR;
}
#endif

INT wifi_getSSIDName(INT apIndex,
                            CHAR *output_string)
{
    const char *interface;
    wifi_util_dbg_print(WIFI_CTRL,"%s:%d: Enter.\n", __func__, __LINE__);

    interface = get_ifname_from_vap_index(apIndex);
    if(!interface) {
      wifi_util_error_print(WIFI_CTRL,"%s:%d:interface for ap index:%d not found\n", __func__, __LINE__, apIndex);
      return RETURN_ERR;
    }

    strncpy(output_string, bss_info[apIndex].ssid, strlen(bss_info[apIndex].ssid) + 1);
    wifi_util_info_print(WIFI_CTRL, "#sherin: %s:%d: output_string: %s\n",__func__, __LINE__, output_string);

    return RETURN_OK;
}

INT wifi_getSSIDMACAddress(INT apIndex,
                            CHAR *output_string)
{
    const char *interface;
    wifi_util_dbg_print(WIFI_CTRL,"%s:%d: Enter.\n", __func__, __LINE__);

    interface = get_ifname_from_vap_index(apIndex);
    if(!interface) {
      wifi_util_error_print(WIFI_CTRL,"%s:%d:interface for ap index:%d not found\n", __func__, __LINE__, apIndex);
      return RETURN_ERR;
    }

    strncpy(output_string, bss_info[apIndex].bssid, strlen(bss_info[apIndex].bssid) + 1);
    wifi_util_info_print(WIFI_CTRL, "#sherin: %s:%d: output_string: %s bssid %s apIndex:%d\n",__func__, __LINE__, output_string, bss_info[apIndex].bssid, apIndex);
    return RETURN_OK;
}

INT wifi_getApEnable(INT apIndex,
                           BOOL *enable)
{
    const char *interface;
    wifi_util_dbg_print(WIFI_CTRL,"%s:%d: Enter.\n", __func__, __LINE__);

    interface = get_ifname_from_vap_index(apIndex);
    if(!interface) {
      wifi_util_error_print(WIFI_CTRL,"%s:%d:interface for ap index:%d not found\n", __func__, __LINE__, apIndex);
      return RETURN_ERR;
    }

    *enable = bss_info[apIndex].enable;
    wifi_util_info_print(WIFI_CTRL, "#sherin: %s:%d: enable: %d\n",__func__, __LINE__, *enable);
    return RETURN_OK;
}

INT get_bss_info (int apIndex, int index, char *vap_name)
{
    FILE *fp;
    char buffer[256];
    char cmd[32] = {0};
    char mac_bssid[32] = {0};

    snprintf(cmd, sizeof(cmd), "hostapd_cli -i wl%d status", index);

    wifi_util_info_print(WIFI_CTRL, "#sherin cmd:%s\n", cmd);
    fp = popen(cmd, "r");

    if (fp == NULL) {
        wifi_util_error_print(WIFI_CTRL, "Failed to run command\n");
        return -1;
    }

    while (fgets(buffer, sizeof(buffer), fp) != NULL) {

        if (strncmp(buffer, "state=", 6) == 0) {

            char *state = strchr(buffer, '=');

            if (state != NULL) {

                state++;
                state[strcspn(state, "\n")] = '\0';
		if (!strncmp(state, "ENABLED", 7))
			bss_info[apIndex].enable = 1;
		else
			bss_info[apIndex].enable = 0;
                wifi_util_info_print(WIFI_CTRL, "state: %s\n", state);

            }
        }

	if (strncmp(buffer, "bssid[", 6) == 0) {

	      char* bssid = strchr(buffer, '=');

		if (bssid != NULL) {

			bssid++; 
			bssid[strcspn(bssid, "\n")] = '\0';
			strncpy (mac_bssid, bssid, strlen(bssid)+1); 

		}
	}

	if (strncmp(buffer, "ssid[", 5) == 0) {

		char *ssid = strchr(buffer, '=');
		wifi_util_info_print(WIFI_CTRL, "#sherin line: %d vap_name %s\n",__LINE__, vap_name);

		if (ssid != NULL) {

			ssid++; 
			ssid[strcspn(ssid, "\n")] = '\0';
			if (strcmp(ssid, vap_name) == 0)
			{
				strncpy(bss_info[apIndex].ssid, ssid, strlen(ssid) + 1);
				wifi_util_info_print(WIFI_CTRL, "#sherin SSID: %s bss_info[%d].ssid %s\n",
						ssid, apIndex, bss_info[apIndex].ssid);
				strncpy(bss_info[apIndex].bssid, mac_bssid, strlen(mac_bssid) + 1);
				wifi_util_info_print(WIFI_CTRL, "#sherin BSSID: %s bss_info[%d].bssid %s\n", mac_bssid, apIndex, bss_info[apIndex].bssid);
				break;
			}
		}
	}
    }

    pclose(fp);


    return RETURN_OK;
}

INT wifi_hal_getRadioVapInfoMap(wifi_radio_index_t index, wifi_vap_info_map_t *map)
{
    wifi_util_info_print(WIFI_CTRL,"%s:%d Inside \n", __func__, __LINE__);
    int apIndex;
    //ret, len, enable;
    int result = RETURN_OK;
    char bssid[32] = { 0 };
    unsigned int i = 0, j = 0;
    unsigned int vap_num = 0;

    if (!g_idx)
    {	  
       init_interface_map ();
       g_idx++;
    }

    while (strlen(interface_index_map[j].vap_name) != 0) {
        if (interface_index_map[j].rdk_radio_index != index) {
            j++;
            continue;
        }

        map->vap_array[i].vap_index = interface_index_map[j].index;

        strncpy(map->vap_array[i].vap_name, interface_index_map[j].vap_name,
            sizeof(interface_index_map[j].vap_name) - 1);
        map->vap_array[i].radio_index = interface_index_map[j].rdk_radio_index;

        apIndex = interface_index_map[j].index;

        if (get_bss_info(apIndex, index, interface_index_map[j].vap_name) < 0){
            wifi_util_error_print(WIFI_CTRL, "%s:%d get_bss_info Failed\n", __func__, __LINE__);
            result = RETURN_ERR;
        }

        if (wifi_getSSIDName(apIndex, map->vap_array[i].u.bss_info.ssid) < 0) {
            wifi_util_error_print(WIFI_CTRL, "%s:%d wifi_getSSIDName Failed\n", __func__, __LINE__);
            result = RETURN_ERR;
        }

        if (wifi_getApEnable(apIndex, &(map->vap_array[i].u.bss_info.enabled)) < 0) {
            wifi_util_error_print(WIFI_CTRL, "%s:%d wifi_getApEnable Failed\n", __func__, __LINE__);
            result = RETURN_ERR;
        }
/*
        if (wifi_getApSsidAdvertisementEnable(apIndex, &(map->vap_array[i].u.bss_info.showSsid)) <
            0) {
            wifi_util_error_print("%s:%d wifi_getApSsidAdvertisementEnable Failed\n", __func__,
                __LINE__);
            result = RETURN_ERR;
        }

        if (wifi_getApManagementFramePowerControl(apIndex,
                &(map->vap_array[i].u.bss_info.mgmtPowerControl)) < 0) {
            wifi_util_error_print("%s:% d wifi_getApManagementFramePowerControl Failed\n", __func__,
                __LINE__);
            result = RETURN_ERR;
        }

        if (wifi_getApMaxAssociatedDevices(apIndex, &(map->vap_array[i].u.bss_info.bssMaxSta)) <
            0) {
            wifi_util_error_print("%s:%d wifi_getApMaxAssociatedDevices Failed\n", __func__,
                __LINE__);
            result = RETURN_ERR;
        }

        if (wifi_getBSSTransitionActivation(apIndex,
                &(map->vap_array[i].u.bss_info.bssTransitionActivated)) < 0) {
            wifi_util_error_print("%s:%d wifi_getBSSTransitionActivation Failed\n", __func__,
                __LINE__);
            result = RETURN_ERR;
        }

        if (wifi_getNeighborReportActivation(apIndex,
                &(map->vap_array[i].u.bss_info.nbrReportActivated)) < 0) {
            wifi_util_error_print("%s:%d  wifi_getNeighborReportActivation Failed\n", __func__,
                __LINE__);
            result = RETURN_ERR;
        }

        ret = wifi_getApSecurity(apIndex, &(map->vap_array[i].u.bss_info.security));
        if (ret != RETURN_OK) {
            result = RETURN_ERR;
        }

        if (wifi_getApInterworkingElement(apIndex,
                &(map->vap_array[i].u.bss_info.interworking.interworking)) < 0) {
            wifi_util_error_print("%s:%d  wifi_getApInterworkingElement Failed\n", __func__,
                __LINE__);
            result = RETURN_ERR;
        }
*/
        if (wifi_getSSIDMACAddress(apIndex, bssid) < 0) {
            wifi_util_error_print(WIFI_CTRL, "%s:%d wifi_getSSIDMACAddress  Failed\n", __func__, __LINE__);
            result = RETURN_ERR;
        }
        if (strlen(bssid) < 17) {
            wifi_util_error_print(WIFI_CTRL, "Invalid BSSID string: %s\n", bssid);
            //result = RETURN_ERR;
        } else {
            MACF_TO_MAC(bssid, map->vap_array[i].u.bss_info.bssid);
            wifi_util_info_print(WIFI_CTRL, "%s:%d: Mac address : %02X:%02X:%02X:%02X:%02X:%02X\n", __func__,
                __LINE__, map->vap_array[i].u.bss_info.bssid[0],
                map->vap_array[i].u.bss_info.bssid[1], map->vap_array[i].u.bss_info.bssid[2],
                map->vap_array[i].u.bss_info.bssid[3], map->vap_array[i].u.bss_info.bssid[4],
                map->vap_array[i].u.bss_info.bssid[5]);
        }
        vap_num++;
        i++;
        j++;
    }
    map->num_vaps = vap_num;
    return result;
}

static int is_hostapd_running_for_iface(const char *iface)
{
    char cmd[256];
    snprintf(cmd, sizeof(cmd),
             "ps | grep hostapd | grep -v grep | grep '%s' > /dev/null",
             iface);
    wifi_util_info_print(WIFI_CTRL,"#sherin: %s:%d cmd: %s\n", __func__, __LINE__, cmd);

    return system(cmd) == 0;
}

static void start_hostapd(const char *config)
{
        pid_t pid = fork();

        wifi_util_info_print(WIFI_CTRL,"#sherin: %s:%d pid: %d\n", __func__, __LINE__, pid);
        if (pid == 0) {
                // Child process
                execlp("hostapd", "hostapd", "-B", "-dd", config, NULL);
                wifi_util_info_print(WIFI_CTRL,"#sherin: %s:%d hostapd started\n", __func__, __LINE__);
                perror("execlp failed");
        }
        else {
                waitpid(pid, NULL, 0);   
        }

}

INT wifi_hal_createVAP(wifi_radio_index_t index, wifi_vap_info_map_t *map)
{
        wifi_util_dbg_print(WIFI_CTRL,"%s:%d Inside radio %d\n", __func__, __LINE__, index);
        wifi_util_info_print(WIFI_CTRL,"#sherin: %s:%d Inside radio %d\n", __func__, __LINE__, index);
        unsigned int i, apIndex;
        FILE *fp;
        FILE *hp;
        char buf[256] = {0};
        char cmd[128] = {0};
        char hostapd_cmd[128] = {0};
        int enabled = 0;

        wifi_vap_info_map_t *tmp_map = (wifi_vap_info_map_t *)malloc(sizeof(wifi_vap_info_map_t));
        memset(tmp_map, 0, sizeof(wifi_vap_info_map_t));
        memcpy(tmp_map, map, sizeof(wifi_vap_info_map_t));

        snprintf(cmd, sizeof(cmd), "/tmp/hostapd_%d.conf", index);

        bool file_exists = (access(cmd, F_OK) == 0);

        hp = fopen(cmd, file_exists ? "a" : "w");
        if (!hp) {
                wifi_util_error_print(WIFI_CTRL,"%s:%d Cannot open %s\n",
                                __func__, __LINE__, cmd);
                return RETURN_ERR;
        }

        if (!file_exists)
        {
                wifi_util_info_print(WIFI_CTRL,"#sherin: %s:%d after file creation %d\n", __func__, __LINE__, index);
                const char *main_if = (index == 0 ? "wl0" : "wl1");

                fprintf(hp, "interface=%s\n", main_if);
                fprintf(hp, "ctrl_interface=/var/run/hostapd\n");
                fprintf(hp, "country_code=GB\n");
                fprintf(hp, "driver=nl80211\n");
                fprintf(hp, "hw_mode=%s\n", (index == 0 ? "g" : "a"));
                fprintf(hp, "channel=%d\n\n", (index == 0 ? 9 : 36 ));
                fprintf(hp, "auth_algs=1\n");
                fprintf(hp, "wpa=0\n");
                fprintf(hp, "ssid=%s\n\n", (index == 0 ? "mesh_sta_2g" : "mesh_sta_5g"));
                fprintf(hp, "# ===== VAP 1 =====\n" "bss=%s.1\n", main_if);
                fprintf(hp, "ssid=%s\n\n", (index == 0 ? "private_ssid_2g" : "private_ssid_5g"));
                fprintf(hp, "# ===== VAP 2 =====\n" "bss=%s.2\n", main_if);
                fprintf(hp, "ssid=%s\n\n", (index == 0 ? "iot_ssid_2g" : "iot_ssid_5g"));
                fprintf(hp, "# ===== VAP 3 =====\n" "bss=%s.7\n", main_if);
                fprintf(hp, "ssid=%s\n\n", (index == 0 ? "mesh_backhaul_2g" : "mesh_backhaul_5g"));
        }
        fclose(hp);
        hp = NULL;

        for (i = 0; i < map->num_vaps; i++) {
                apIndex = map->vap_array[i].vap_index;
                const char *vap_name = get_ifname_from_vap_index (apIndex);
                wifi_util_info_print (WIFI_CTRL, "#sherin: apIndex:%d\n", apIndex, map->vap_array[i].u.bss_info.enabled);
                if (!map->vap_array[i].u.bss_info.enabled) {
                        wifi_util_info_print(WIFI_CTRL,"#sherin: %s:%d ap_index:%d not enabled\n", __func__, __LINE__, apIndex);

                        snprintf(cmd, sizeof(cmd),
                                        "ip link show wl%d.%d 2>/dev/null", index, apIndex);
                        fp = popen(cmd, "r");
                        if (!fp) {
                                return -1;
                        }

                        wifi_util_info_print(WIFI_CTRL,"#sherin: %s:%d:\n", __func__, __LINE__);
                        while (fgets(buf, sizeof(buf), fp) != NULL) {
                                if (strstr(buf, "UP")) {
                                        enabled = 1;
                                        break;
                                }
                        }

                        pclose(fp);
                        fp = NULL;

                        wifi_util_info_print(WIFI_CTRL,"#sherin: %s:%d:\n", __func__, __LINE__);
                        if (!enabled)
                        {
                                memset(cmd,0,sizeof(cmd));
                                snprintf (cmd, sizeof(cmd), "ip link set %s down", vap_name);
                                wifi_util_info_print(WIFI_CTRL,"#sherin: %s:%d cmd: %s\n", __func__, __LINE__, cmd);
                                system(cmd);
                        }
                        continue;
                }
                /*
                   memset(cmd,0,sizeof(cmd));
                   sprintf (cmd, "iw dev wl%d interface add wl%d.%d type __ap", index, index, apIndex);
                   system(cmd);
                   */

                memset(cmd,0,sizeof(cmd));

                snprintf (cmd, sizeof(cmd), "ip link set %s up",vap_name);
                system(cmd);
        }

        wifi_util_info_print(WIFI_CTRL,"%s:%d Outside \n", __func__, __LINE__);
        //snprintf (hostapd_cmd, sizeof(hostapd_cmd),"hostapd -B -dd /tmp/hostapd_%d.conf",index);

        snprintf (hostapd_cmd, sizeof(hostapd_cmd),"/tmp/hostapd_%d.conf",index);
        wifi_util_info_print(WIFI_CTRL,"%s:%d cmd:%s \n", __func__, __LINE__, hostapd_cmd);

	if (!is_hostapd_running_for_iface(hostapd_cmd))
	{
            start_hostapd (hostapd_cmd);
	    sleep(5);
        }

        memset(cmd,0,sizeof(cmd));
        memset(buf,0,sizeof(buf));
        snprintf (cmd,sizeof(cmd), "hostapd_cli -i wl%d status",index);
        fp = popen(cmd, "r");
        if (fp == NULL) {
                perror("popen failed");
                return 1;
        }


        enabled = 0;

        while (fgets(buf, sizeof(buf), fp) != NULL) {
                if (strstr(buf, "state=ENABLED")) {
                        enabled = 1;
                        break;
                }
        }
        pclose(fp);
        fp = NULL;
        if (!enabled) {
                wifi_util_error_print(WIFI_CTRL,"%s:%d wifi_setApEnable Failed\n", __func__, __LINE__);
                return RETURN_ERR;
        }
        free(tmp_map);
        return RETURN_OK;
}

