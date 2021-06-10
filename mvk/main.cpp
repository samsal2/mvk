#include "renderer.hpp"
#include "utility/verify.hpp"

int
main()
{
        try
        {
                auto rdr = mvk::renderer();
                rdr.init();
                rdr.run();
        }
        catch (mvk::utility::verify_error error)
        {
                std::cerr << error.what() << '\n';
        }

        return 0;
}
