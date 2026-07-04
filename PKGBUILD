# Maintainer: metricd <https://github.com/Arch-Noah/metricd>

pkgname=metricd
pkgver=1.0.0
pkgrel=1
pkgdesc="Ultra-lightweight system metrics daemon with Unix socket IPC"
arch=('x86_64')
url="https://github.com/Arch-Noah/metricd"
license=('MIT')
depends=('liburing' 'gcc-libs')
makedepends=('cmake' 'nlohmann-json')
install=metricd.install

source=("${pkgname}-${pkgver}.tar.gz::https://github.com/Arch-Noah/metricd/archive/v${pkgver}.tar.gz")
sha256sums=('SKIP')

build() {
    cd "${srcdir}/${pkgname}-${pkgver}"
    cmake -B build \
        -DCMAKE_BUILD_TYPE=Release \
        -DCMAKE_INSTALL_PREFIX=/usr
    cmake --build build -j"$(nproc)"
}

check() {
    cd "${srcdir}/${pkgname}-${pkgver}"
    ctest --test-dir build
}

package() {
    cd "${srcdir}/${pkgname}-${pkgver}"
    install -Dm755 build/src/metricd "${pkgdir}/usr/bin/metricd"
    install -Dm644 config/metricd.default.toml \
        "${pkgdir}/usr/share/metricd/metricd.default.toml"
    install -Dm644 packaging/systemd/metricd.service \
        "${pkgdir}/usr/lib/systemd/user/metricd.service"
    install -Dm644 LICENSE "${pkgdir}/usr/share/licenses/${pkgname}/LICENSE"
    install -Dm644 docs/ARCHITECTURE.md "${pkgdir}/usr/share/doc/${pkgname}/ARCHITECTURE.md"
    install -Dm644 docs/PROTOCOL.md "${pkgdir}/usr/share/doc/${pkgname}/PROTOCOL.md"
}
