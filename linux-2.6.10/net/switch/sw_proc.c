#include <linux/proc_fs.h>

#include "sw_private.h"
#include "sw_debug.h"
#include "sw_proc.h"

#define SCR_RIGHT_MAX 70

static struct proc_dir_entry *switch_dir, *iface_file,
							 *mac_file;


static int read_vlan_bitmap(char *page, struct net_switch_port *port, int initial_offset) {
	int i, vlan, min, max, mask, count;
	int flag=0;
	int offset = initial_offset;
	
	// listare allowed vlans
	min = max = vlan = count = 0;
	for (i=0; i<SW_VLAN_BMP_NO; i++) {
		for (mask=0x01; mask < 0x100; mask<<=1, vlan++) {
			if (! (port->forbidden_vlans[i] & mask)) {
				min = max = vlan;
				mask <<= 1;
				flag = 1;
				break;
			}
		}
		if (flag) break;
	}
	if (!flag) return count;
	
	vlan++;
	for (;i<SW_VLAN_BMP_NO; i++) {
		for (; mask < 0x100; mask<<=1, vlan++) {
			if ((port->forbidden_vlans[i] & mask) && flag) {
				if (offset > initial_offset) offset+=sprintf(page+count+offset, ", ");
				if (offset > SCR_RIGHT_MAX) {
					count+=offset;
					count+=sprintf(page+count, "\n");
					for (offset=0; offset<initial_offset; )
						offset+=sprintf(page+count+offset," ");
				}
				if (max - min > 1) 
					offset += sprintf(page+count+offset, "%d-%d", min, max);
				else if (max - min == 1) 
					offset += sprintf(page+count+offset, "%d, %d", min, max);
				else 
					offset += sprintf(page+count+offset, "%d", min);
				flag = 0;
			}
			else if (! (port->forbidden_vlans[i] & mask)) {
				if (!flag) {
					min = max = vlan;
					flag = 1;
				}
				else {
					max = vlan;
				}
			}
		}
		mask = 0x01;
	}

	if (! (port->forbidden_vlans[SW_VLAN_BMP_NO-1] & 0x80)) {
		if (offset > initial_offset) offset+=sprintf(page+count+offset, ", ");
		if (offset > SCR_RIGHT_MAX) {
			count+=offset;
			count+=sprintf(page+count, "\n");
			for (offset=0; offset<initial_offset;)
				offset+=sprintf(page+count+offset," ");
		}
		if (max - min > 1) 
			offset += sprintf(page+count+offset, "%d-%d", min, max);
		else if (max - min == 1)
			offset += sprintf(page+count+offset, "%d, %d", min, max);
		else 
			offset += sprintf(page+count+offset, "%d", min);
	}

	count+=offset;

	return count;
}

static int proc_read_ifaces(char *page, char **start,
		off_t off, int count,
		int *eof, void *data) {
		
	struct net_switch_port *port;
	int len = 0;
	
	len += sprintf(page, "Port  Trunk  Enabled  VLAN\n"
		"----  -----  -------  ----\n");

	rcu_read_lock();
	list_for_each_entry(port, &sw.ports, lh) {
		len+= sprintf(page+len, "%4s  %5d  %7d  ",
			port->dev->name, port->flags & SW_PFL_TRUNK, 
			(port->flags & SW_PFL_DISABLED) == 0);
		if (port->flags & SW_PFL_TRUNK) {
			len += read_vlan_bitmap(page+len, port, 22);
			len += sprintf(page+len, "\n");
		}
		else 
			len += sprintf(page+len, "%4d\n", port->vlan);
	}
	rcu_read_unlock();
	return len;
}

static int proc_read_mac(char *page, char **start,
		off_t off, int count,
		int *eof, void *data) {
		
	struct net_switch_fdb_entry *entry;	
	int len = 0;
	int i;

	len += sprintf(page, "Destination Address  Address Type  VLAN  Destination Port\n"
		"-------------------  ------------  ----  ----------------\n");
	for (i=0; i<SW_HASH_SIZE; i++) {
		read_lock(&sw.fdb[i].lock);
		list_for_each_entry(entry, &sw.fdb[i].entries, lh) {
			len+=sprintf(page+len, "%02x%02x.%02x%02x.%02x%02x       "
				"Dynamic       %4d  %s\n",
				entry->mac[0], entry->mac[1], entry->mac[2],
				entry->mac[3], entry->mac[4], entry->mac[5],
				entry->vlan,
				entry->port->dev->name
				);
		}
		read_unlock(&sw.fdb[i].lock);
	}
		
	return len;
}

int init_switch_proc(void) {

	/* Create our own directory under /proc/net */
	switch_dir = proc_mkdir(SW_PROCFS_DIR, proc_net);
	if (!switch_dir)
		return -ENOMEM;

	switch_dir->owner = THIS_MODULE;

	/*
	   Create the ifaces file, which lists information
	   about the interfaces added to switch
	 */
	iface_file = create_proc_read_entry(SW_PROCFS_IFACES, 0600,
			switch_dir, proc_read_ifaces, NULL);
	if (!iface_file) {
		remove_proc_entry(SW_PROCFS_DIR, proc_net);
		return -ENOMEM;
	}
	iface_file->owner = THIS_MODULE;

	mac_file = create_proc_read_entry(SW_PROCFS_MAC, 0600,
			switch_dir, proc_read_mac, NULL);
	if (!mac_file) {
		remove_proc_entry(SW_PROCFS_DIR, proc_net);
		remove_proc_entry(SW_PROCFS_IFACES, switch_dir);
		return -ENOMEM;
	}
	mac_file->owner = THIS_MODULE;

	dbg("sw_proc initialized\n");
	return 0;
}

void cleanup_switch_proc(void) {
	remove_proc_entry(SW_PROCFS_IFACES, switch_dir);
	remove_proc_entry(SW_PROCFS_MAC, switch_dir);
	remove_proc_entry(SW_PROCFS_DIR, proc_net);
}
