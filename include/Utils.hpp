#include<stdexcept>

namespace ENG {
template<typename Tfrom, typename Tto>
Tto* checked_cast(Tfrom* base) {
    Tto* ptr = dynamic_cast<Tto*>(base);
    if (!ptr) throw std::runtime_error("Invalid cast");
    return ptr;
}
}
