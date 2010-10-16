/*
    Pynetinfo - A python module for controlling linux network interfaces
    Copyright (C) 2010  Sassan Panahinejad (sassan@sassan.me.uk) 
    www.sassan.me.uk
    pypi.python.org/pypi/pynetinfo/

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
*/

#include <Python.h>
#include <stdio.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <linux/sockios.h>
#include <net/if.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <net/route.h>

// add_route(dev, destination, gateway, netmask)
PyObject *netinfo_adddel_route(PyObject *self, PyObject *args, int action)
{
    int ret, fd;
    char *dev, *dest, *gateway, *netmask;
    struct sockaddr_in *sin;
    struct rtentry rtentry;
    fd = socket(AF_INET, SOCK_DGRAM, 0); /* open a socket to examine */
    if (fd < 0) {
        PyErr_SetFromErrno(PyExc_Exception);
        return NULL;
    }
    ret = PyArg_ParseTuple(args, "ssss", &dev, &dest, &gateway, &netmask); /* parse argument */
    if (!ret)
        return NULL;
    /* set up route entry */ 
    memset(&rtentry, 0, sizeof(struct rtentry));
    if (strlen(dev) > 0) /* null if no device specified (os guesses) */
        rtentry.rt_dev = dev;
    rtentry.rt_metric = 1;
    rtentry.rt_flags = RTF_UP;
    /* build destination */
    rtentry.rt_dst.sa_family = AF_INET;
    sin = (struct sockaddr_in *)&rtentry.rt_dst;
    inet_aton(dest, &sin->sin_addr);
    /* build gateway */
    rtentry.rt_gateway.sa_family = AF_INET;
    sin = (struct sockaddr_in *)&rtentry.rt_gateway;
    inet_aton(gateway, &sin->sin_addr);
    if (sin->sin_addr.s_addr)
        rtentry.rt_flags |= RTF_GATEWAY;
    /* build netmask */
    rtentry.rt_genmask.sa_family = AF_INET;
    sin = (struct sockaddr_in *)&rtentry.rt_genmask;
    inet_aton(netmask, &sin->sin_addr);
    if (sin->sin_addr.s_addr == 0xFFFFFFFF)
        rtentry.rt_flags |= RTF_HOST;
    /* add route */
    ret = ioctl(fd, action ? SIOCADDRT : SIOCDELRT, &rtentry);
    if (ret < 0) {
        PyErr_SetFromErrno(PyExc_Exception);
        return NULL;
    }
    return Py_None;
}

PyObject *netinfo_add_route(PyObject *self, PyObject *args)
{
    return netinfo_adddel_route(self, args, 1);
}

PyObject *netinfo_del_route(PyObject *self, PyObject *args)
{
    return netinfo_adddel_route(self, args, 0);
}

PyObject *netinfo_get_routes(PyObject *self, PyObject *args)
{
    char buffer[1024], *tok, *c, *ipstr;
    int field = 0, i = 1;
    struct in_addr addr;
    PyObject *dict, *tuple = PyTuple_New(0);
    FILE *file = fopen("/proc/net/route", "r");

    if (!file) {
        PyErr_SetFromErrno(PyExc_Exception);
        return NULL;
    }
    if (!fgets(buffer, 1024, file)) {
        PyErr_SetFromErrno(PyExc_Exception);
        return NULL;
    }
//     strtok_r(buffer, " \t", &tok);
    while (fgets(buffer, 1024, file)) {
        dict = PyDict_New();
        field = 0;
        while ((c = strtok_r(field ? NULL : buffer, " \t", &tok))) {
            switch (field++) {
                case 0:
//                     printf("iface: %s\n", c);
                    PyDict_SetItemString(dict, "dev", Py_BuildValue("s", c));
                    break;
                case 1:
//                     printf("dest: %s\n", c);
                    sscanf(c, "%X", (uint32_t *)&addr);
                    ipstr = inet_ntoa(addr);
                    PyDict_SetItemString(dict, "dest", Py_BuildValue("s", ipstr));
                    break;
                case 2:
//                     printf("gateway: %s\n", c);
                    sscanf(c, "%X", (uint32_t *)&addr);
                    ipstr = inet_ntoa(addr);
                    PyDict_SetItemString(dict, "gateway", Py_BuildValue("s", ipstr));
                    break;  
                case 7:
//                     printf("netmask: %s\n", c);
                    sscanf(c, "%X", (uint32_t *)&addr);
                    ipstr = inet_ntoa(addr);
                    PyDict_SetItemString(dict, "netmask", Py_BuildValue("s", ipstr));
                    break;  
                default:  
                    break;
            }
        }
        _PyTuple_Resize(&tuple, i);
        PyTuple_SET_ITEM(tuple, i++-1, dict);
    }
    return tuple;
}

