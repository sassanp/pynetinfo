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

#include "netinfo.h"

#define IF_COUNT 64

PyObject *netinfo_list_active_devs(PyObject *self, PyObject *args)
{
    int ret, fd;
    struct ifreq *ifr, *ifend;
    struct ifreq ifs[IF_COUNT];
    struct ifconf ifc;
    int i = 1;
    PyObject *tuple = PyTuple_New(0);

    fd = socket(AF_INET, SOCK_DGRAM, IPPROTO_IP); /* open a socket to examine */
    if (fd < 0) {
        PyErr_SetFromErrno(PyExc_Exception);
        return NULL;
    }
    ifc.ifc_len = sizeof(ifs);
    ifc.ifc_req = ifs;

    ret = ioctl(fd, SIOCGIFCONF, &ifc);
    if (ret < 0) {
        PyErr_SetFromErrno(PyExc_Exception);
        close(fd);
        return NULL;
    }
    ifend = ifs + (ifc.ifc_len / sizeof(struct ifreq));
    for (ifr = ifc.ifc_req; ifr < ifend; ifr++)
    {
//         printf("dev: %s\n", ifr->ifr_name);
        _PyTuple_Resize(&tuple, i);
        PyTuple_SET_ITEM(tuple, i++-1, Py_BuildValue("s", ifr->ifr_name));
    }
    close(fd);
    return tuple;
}

PyObject *netinfo_list_devs(PyObject *self, PyObject *args)
{
    FILE *devlist = fopen("/proc/net/dev", "r");
    char buffer[256], *c, *end;
    int i = 1;
    PyObject *tuple = PyTuple_New(0);
    while (fgets(buffer, 256, devlist)) {
        end = strchr(buffer, ':');
        if (!end)
            continue;
        *end = '\0';
        for (c = buffer; *c == ' '; c++) ;
//         printf("dev: %s\n", c);
        _PyTuple_Resize(&tuple, i);
        PyTuple_SET_ITEM(tuple, i++-1, Py_BuildValue("s", c));
    }
    fclose(devlist);
    return tuple;
}

PyObject *netinfo_get_addr(PyObject *self, PyObject *args, int cmd)
{
    int ret, fd;
    struct ifreq ifreq;
    char *dev;
    struct sockaddr_in *sin;
    char hwaddr[18];
    fd = socket(AF_INET, SOCK_DGRAM, 0); /* open a socket to examine */
    if (fd < 0) {
        PyErr_SetFromErrno(PyExc_Exception);
        return NULL;
    }

    ret = PyArg_ParseTuple(args, "s", &dev); /* parse argument */
    if (!ret) {
        close(fd);
        return NULL;
    }

    memset(&ifreq, 0, sizeof(struct ifreq));
    strncpy(ifreq.ifr_name, dev, IFNAMSIZ-1);
    ifreq.ifr_addr.sa_family = AF_INET;
    ret = ioctl(fd, cmd, &ifreq, sizeof(struct ifreq));
    if (ret < 0) {
        PyErr_SetFromErrno(PyExc_Exception);
        close(fd);
        return NULL;
    }
    
    close(fd);  // remember to close the socket

    switch (cmd) {
        case SIOCGIFADDR:
            sin = (struct sockaddr_in *)&(ifreq.ifr_ifru.ifru_addr);
            return Py_BuildValue("s", inet_ntoa(sin->sin_addr));
        case SIOCGIFNETMASK:
            sin = (struct sockaddr_in *)&(ifreq.ifr_ifru.ifru_netmask);
            return Py_BuildValue("s", inet_ntoa(sin->sin_addr));
        case SIOCGIFBRDADDR:
            sin = (struct sockaddr_in *)&(ifreq.ifr_ifru.ifru_broadaddr);
            return Py_BuildValue("s", inet_ntoa(sin->sin_addr));
        case SIOCGIFHWADDR:
            snprintf(hwaddr, 18, "%02X:%02X:%02X:%02X:%02X:%02X", 
                        (unsigned char)ifreq.ifr_ifru.ifru_hwaddr.sa_data[0],
                        (unsigned char)ifreq.ifr_ifru.ifru_hwaddr.sa_data[1],
                        (unsigned char)ifreq.ifr_ifru.ifru_hwaddr.sa_data[2],
                        (unsigned char)ifreq.ifr_ifru.ifru_hwaddr.sa_data[3],
                        (unsigned char)ifreq.ifr_ifru.ifru_hwaddr.sa_data[4],
                        (unsigned char)ifreq.ifr_ifru.ifru_hwaddr.sa_data[5]);
            return Py_BuildValue("s", hwaddr);
    }
    return NULL;
}

PyObject *netinfo_get_ip(PyObject *self, PyObject *args)
{
    return netinfo_get_addr(self, args, SIOCGIFADDR);
}

PyObject *netinfo_get_netmask(PyObject *self, PyObject *args)
{
    return netinfo_get_addr(self, args, SIOCGIFNETMASK);
}

PyObject *netinfo_get_broadcast(PyObject *self, PyObject *args)
{
    return netinfo_get_addr(self, args, SIOCGIFBRDADDR);
}

PyObject *netinfo_get_hwaddr(PyObject *self, PyObject *args)
{
    return netinfo_get_addr(self, args, SIOCGIFHWADDR);
}



PyObject *netinfo_set_state(PyObject *self, PyObject *args)
{
    int ret, fd, state = 0;
    struct ifreq ifreq;
    char *dev;
    ret = PyArg_ParseTuple(args, "si", &dev, &state); /* parse argument */
    if (!ret)
        return NULL;
//     ret = PyArg_ParseTuple(args, "i", &state); /* parse argument */
//     if (!ret)
//         return NULL;
    fd = socket(AF_INET, SOCK_DGRAM, IPPROTO_IP); /* open a socket to examine */
    if (fd < 0) {
        PyErr_SetFromErrno(PyExc_Exception);
        return NULL;
    }
    memset(&ifreq, 0, sizeof(struct ifreq));
    strncpy(ifreq.ifr_name, dev, IFNAMSIZ-1);
    ret = ioctl(fd, SIOCGIFFLAGS, &ifreq);
    if (ret < 0) {
        PyErr_SetFromErrno(PyExc_Exception);
        close(fd);
        return NULL;
    }
    if (state)
        ifreq.ifr_flags |= IFF_UP;
    else
        ifreq.ifr_flags &= ~IFF_UP;
    ret = ioctl(fd, SIOCSIFFLAGS, &ifreq);
    if (ret < 0) {
        PyErr_SetFromErrno(PyExc_Exception);
        close(fd);
        return NULL;
    }
    close(fd);
    return Py_None;
}

PyObject *netinfo_set_addr(PyObject *self, PyObject *args, int cmd)
{
    int ret, fd;
    struct ifreq ifreq;
    char *dev, *addr;
    struct sockaddr_in *sin;
    fd = socket(AF_INET, SOCK_DGRAM, 0); /* open a socket to examine */
    if (fd < 0) {
        PyErr_SetFromErrno(PyExc_Exception);
        return NULL;
    }
    ret = PyArg_ParseTuple(args, "ss", &dev, &addr); /* parse argument */
    if (!ret) {
        close(fd);
        return NULL;
    }
    memset(&ifreq, 0, sizeof(struct ifreq));
    strncpy(ifreq.ifr_name, dev, IFNAMSIZ-1);
    ifreq.ifr_addr.sa_family = AF_INET;
    switch (cmd) {
        case SIOCSIFADDR:
            sin = (struct sockaddr_in *)&(ifreq.ifr_ifru.ifru_addr);
            inet_aton(addr, &sin->sin_addr);
            break;
        case SIOCSIFNETMASK:
            sin = (struct sockaddr_in *)&(ifreq.ifr_ifru.ifru_netmask);
            inet_aton(addr, &sin->sin_addr);
            break;
        case SIOCSIFBRDADDR:
            sin = (struct sockaddr_in *)&(ifreq.ifr_ifru.ifru_broadaddr);
            inet_aton(addr, &sin->sin_addr);
            break;
    }
    ret = ioctl(fd, cmd, &ifreq, sizeof(struct ifreq));
    if (ret < 0) {
        PyErr_SetFromErrno(PyExc_Exception);
        close(fd);
        return NULL;
    }
    close(fd);
    return Py_None;
}

PyObject *netinfo_set_ip(PyObject *self, PyObject *args)
{
    return netinfo_set_addr(self, args, SIOCSIFADDR);
}

PyObject *netinfo_set_netmask(PyObject *self, PyObject *args)
{
    return netinfo_set_addr(self, args, SIOCSIFNETMASK);
}

PyObject *netinfo_set_broadcast(PyObject *self, PyObject *args)
{
    return netinfo_set_addr(self, args, SIOCSIFBRDADDR);
}




