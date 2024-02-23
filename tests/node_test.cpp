#include "test.hpp"

TEST(Node, ReadWriteNode) {
    auto file = util::MakePtr<mem::File>("test.data", CONSOLE_LOGGER);
    file->Clear();
    auto coords =
        ts::NewClass<ts::StructClass>("coords", ts::NewClass<ts::PrimitiveClass<double>>("lat"),
                                      ts::NewClass<ts::PrimitiveClass<double>>("lon"));
    auto meta1 = db::Node(mem::kMagic, 0, ts::New<ts::Struct>(coords, 0., 0.));
    meta1.Write(file, 0);
    auto meta2 = db::Node(mem::kMagic, coords, file, 0);
    std::cerr << meta1.ToString() << std::endl;
    ASSERT_EQ(meta1.ToString(), meta2.ToString());
}

TEST(Node, NodeVarious) {
    auto file = util::MakePtr<mem::File>("test.data", CONSOLE_LOGGER);
    file->Clear();
    auto coords =
        ts::NewClass<ts::StructClass>("coords", ts::NewClass<ts::PrimitiveClass<double>>("lat"),
                                      ts::NewClass<ts::PrimitiveClass<double>>("lon"));
    auto string = ts::NewClass<ts::StringClass>("string");

    auto meta_coords = db::Node(mem::kMagic, 0, ts::New<ts::Struct>(coords, 0., 0.));
    meta_coords.Write(file, 0);
    file->Write(~mem::kMagic, 0);
    auto meta_coords_free = db::Node(mem::kMagic, coords, file, 0);
    file->Write<size_t>(0, 0);
    auto meta_coords_ivalid = db::Node(mem::kMagic, coords, file, 0);

    std::cerr << meta_coords.ToString() << std::endl;
    std::cerr << meta_coords_free.ToString() << std::endl;
    std::cerr << meta_coords_ivalid.ToString() << std::endl;

    auto meta_string = db::Node(mem::kMagic, 1, ts::New<ts::String>(string, "bebra"));
    meta_string.Write(file, 0);
    file->Write(~mem::kMagic, 0);
    auto meta_string_free = db::Node(mem::kMagic, string, file, 0);
    file->Write<size_t>(0, 0);
    auto meta_string_ivalid = db::Node(mem::kMagic, string, file, 0);

    std::cerr << meta_string.ToString() << std::endl;
    std::cerr << meta_string_free.ToString() << std::endl;
    std::cerr << meta_string_ivalid.ToString() << std::endl;
}