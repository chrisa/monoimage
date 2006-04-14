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
