#
#  License:
#
#  Copyright (c) 2013 AlienVault
#  All rights reserved.
#
#  This package is free software; you can redistribute it and/or modify
#  it under the terms of the GNU General Public License as published by
#  the Free Software Foundation; version 2 dated June, 1991.
#  You may not use, modify or distribute this program under any other version
#  of the GNU General Public License.
#
#  This package is distributed in the hope that it will be useful,
#  but WITHOUT ANY WARRANTY; without even the implied warranty of
#  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#  GNU General Public License for more details.
#
#  You should have received a copy of the GNU General Public License
#  along with this package; if not, write to the Free Software
#  Foundation, Inc., 51 Franklin St, Fifth Floor, Boston,
#  MA  02110-1301  USA
#
#
#  On Debian GNU/Linux systems, the complete text of the GNU General
#  Public License can be found in `/usr/share/common-licenses/GPL-2'.
#
#  Otherwise you can read it here: http://www.gnu.org/licenses/gpl-2.0.txt
#
import syslog


def info(message):
    syslog.syslog(syslog.LOG_INFO, "ALIENVAULT-API[INFO]: " + message)


def debug(message):
    syslog.syslog(syslog.LOG_DEBUG, "ALIENVAULT-API[DEBUG]: " + message)


def warning(message):
    syslog.syslog(syslog.LOG_WARNING, "ALIENVAULT-API[WARNING]: " + message)


def error(message):
    syslog.syslog(syslog.LOG_ERR, "ALIENVAULT-API[ERROR]: " + message)


def critical(message):
    syslog.syslog(syslog.LOG_CRIT, "ALIENVAULT-API[CRITICAL]: " + message)

log_level = {
    'INFO': info,
    'DEBUG': debug,
    'WARNING': warning,
    'ERROR': error,
    'CRITICAL': critical
}


def log(message, level='INFO'):
    try:
        log_level[level](message)
    except KeyError:
        log_level['INFO']("Bad log level '%s'" % level)
        log_level['INFO'](message)
