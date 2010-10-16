#    Pynetinfo - A python module for controlling linux network interfaces
#    Copyright (C) 2010  Sassan Panahinejad (sassan@sassan.me.uk) 
#    www.sassan.me.uk
#    pypi.python.org/pypi/pynetinfo/

#    This program is free software; you can redistribute it and/or modify
#    it under the terms of the GNU General Public License as published by
#    the Free Software Foundation; either version 2 of the License, or
#    (at your option) any later version.

#    This program is distributed in the hope that it will be useful,
#    but WITHOUT ANY WARRANTY; without even the implied warranty of
#    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#    GNU General Public License for more details.

#    You should have received a copy of the GNU General Public License
#    along with this program; if not, write to the Free Software
#    Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA

import netinfo
for dev in netinfo.list_active_devs():
	print "*********************************"
	print dev
	print "mac: "+netinfo.get_hwaddr(dev)
	print "ip: "+netinfo.get_ip(dev)
	print "netmask: "+netinfo.get_netmask(dev)
	print "broadcast: "+netinfo.get_broadcast(dev)
print "######################################"
for route in netinfo.get_routes():
    print route['dest']+"/"+route['netmask']+" -> "+route['gateway']


#netinfo.del_route("", "192.168.0.0", "164.11.222.253", "255.255.255.0");
