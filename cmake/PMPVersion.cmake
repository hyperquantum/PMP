
# define version information
set(PMP_VERSION_MAJOR 0)
set(PMP_VERSION_MINOR 0)
set(PMP_VERSION_PATCH 7)

# define branding information
set(PMP_ORGANIZATION_NAME "Party Music Player")
set(PMP_ORGANIZATION_DOMAIN "hyperquantum.be")
set(PMP_WEBSITE "http://hyperquantum.be/pmp")
set(PMP_BUGREPORT_LOCATION "https://github.com/hyperquantum/PMP/issues")
set(PMP_COPYRIGHT_YEARS "2014-2022")

# create derived variables

set(
    PMP_VERSION_MAJORMINORPATCH
    ${PMP_VERSION_MAJOR}.${PMP_VERSION_MINOR}.${PMP_VERSION_PATCH}
)
set(PMP_VERSION_DISPLAY "${PMP_VERSION_MAJORMINORPATCH}")
