# Copyright 2023 Elijah Gordon (SLcK) <braindisassemblue@gmail.com>

# This program is free software; you can redistribute it and/or
# modify it under the terms of the GNU General Public License
# as published by the Free Software Foundation; either version 2
# of the License, or (at your option) any later version.

# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.

# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software
# Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.

.SILENT:

bin_PROGRAMS = illumiterm
illumiterm_SOURCES = illumiterm.c

illumiterm_CFLAGS = @GTK_CFLAGS@ @VTE_CFLAGS@ $(MORE_CFLAGS)
illumiterm_LDFLAGS = @GTK_LIBS@ @VTE_LIBS@

icondir_48 = /usr/share/icons/hicolor/48x48/apps
icondir_96 = /usr/share/icons/hicolor/96x96/apps
bindir = $(prefix)/bin

install-data-local:
	$(INSTALL_DATA) $(top_srcdir)/icons/illumiterm.png $(DESTDIR)$(icondir_48)/illumiterm.png
	$(INSTALL_DATA) $(top_srcdir)/icons/about.png $(DESTDIR)$(icondir_96)/about.png
	$(INSTALL_DATA) $(top_srcdir)/illumiterm.desktop.in $(DESTDIR)$(desktopdir)/illumiterm.desktop
	
uninstall-local:
	$(RM) $(DESTDIR)$(icondir_48)/illumiterm.png
	$(RM) $(DESTDIR)$(icondir_96)/about.png
	$(RM) $(DESTDIR)$(desktopdir)/illumiterm.desktop

install-exec-hook:
	$(INSTALL_PROGRAM) illumiterm $(DESTDIR)$(bindir)

uninstall-hook:
	$(RM) $(DESTDIR)$(bindir)/illumiterm
