# Copyright 1999-2006 Gentoo Foundation
# Distributed under the terms of the GNU General Public License v2
# $Header: /home/cvsroot/suppose-aldigital-portage/net-misc/pc300/pc300-4.1.0.ebuild,v 1.2 2006/04/10 18:00:51 dunc Exp $

DESCRIPTION="kexec-tools"
HOMEPAGE="http://www.xmission.com/"

MY_P="kexec-tools-1.8"
SRC_URI="http://www.xmission.com/~ebiederm/files/kexec/${MY_P}.tar.gz"

LICENSE="GPL"
SLOT="0"
KEYWORDS="x86"
IUSE=""

DEPEND=""
RDEPEND=""

src_compile() {
	cd "${S}"
	einfo "Compiling"
	emake -j1 || die "${diemsg}"
}

src_install() {
	cd "${S}"
	mkdir "${D}/sbin"
	mkdir "${D}/bin"
	install -m 755 objdir/build/sbin/kexec "${D}/sbin/kexec"
	install -m 755 objdir/build/bin/kexec_test "${D}/bin/kexec_test"
}
