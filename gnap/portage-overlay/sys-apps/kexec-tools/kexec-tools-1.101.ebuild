# Copyright 1999-2005 Gentoo Foundation
# Distributed under the terms of the GNU General Public License v2
# $Header: /var/cvsroot/gentoo-x86/sys-apps/kexec-tools/kexec-tools-1.101.ebuild,v 1.3 2005/12/01 08:13:23 vapier Exp $

inherit eutils

DESCRIPTION="Load another kernel from the currently executing Linux kernel \
(with optional patch for monoimage support)"
HOMEPAGE="http://www.xmission.com/~ebiederm/files/kexec/"
SRC_URI="http://www.xmission.com/~ebiederm/files/kexec/${P}.tar.gz"

LICENSE="GPL-2"
SLOT="0"
KEYWORDS="~amd64 x86"
IUSE="monoimage"

src_unpack() {
	unpack ${A}
	cd ${S}

	use monoimage && epatch ${FILESDIR}/${P}-monoimage.patch
}

src_compile() {
	local myconf
	use monoimage && myconf="--with-libext2fs"
	econf ${myconf} || die
	emake || die
}

src_install() {
	einstall  || die
}

