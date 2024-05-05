
#include "relation.hpp"

#include <ostream>

#include "test.hpp"

TEST(Relation, RelationClass) {
    auto file = util::MakePtr<mem::File>("test.data");

    auto name = ts::NewClass<ts::StringClass>("name");
    auto age = ts::NewClass<ts::PrimitiveClass<int>>("age");

    auto name_to_age = util::MakePtr<ts::RelationClass>("name-to-age", name, age);

    auto holder = util::MakePtr<ts::ClassObject>(name_to_age);
    std::cerr << holder->ToString() << std::endl;
    auto old = holder->ToString();
    holder->Write(file, 0);
    holder->Read(file, 0);
    std::cerr << holder->ToString() << std::endl;
    ASSERT_EQ(old, holder->ToString());

    auto name_to_age_aged = util::MakePtr<ts::RelationClass>("name-to-age-aged", name, age, age);
    holder = util::MakePtr<ts::ClassObject>(name_to_age_aged);
    old = holder->ToString();
    std::cerr << old << std::endl;
    holder->Write(file, 0);
    holder->Read(file, 0);
    std::cerr << holder->ToString() << std::endl;
    ASSERT_EQ(old, holder->ToString());
}

TEST(Relation, RelationObject) {
    auto file = util::MakePtr<mem::File>("test.data");

    auto name = ts::NewClass<ts::StringClass>("name");
    auto age = ts::NewClass<ts::PrimitiveClass<int>>("age");

    auto name_to_age = util::MakePtr<ts::RelationClass>("name-to-age", name, age);

    auto object = util::MakePtr<ts::Relation>(name_to_age, 1, 1);
    std::cerr << object->ToString() << std::endl;
    auto old = object->ToString();
    object->Write(file, 0);
    object->Read(file, 0);
    std::cerr << object->ToString() << std::endl;
    ASSERT_EQ(old, object->ToString());

    auto name_to_age_aged = util::MakePtr<ts::RelationClass>("name-to-age-aged", name, age, age);
    object = util::MakePtr<ts::Relation>(name_to_age_aged, 2, 2,
                                         ts::DefaultNew<ts::Primitive<int>>(age));
    std::cerr << object->ToString() << std::endl;
    old = object->ToString();
    object->Write(file, 0);
    object->Read(file, 0);
    std::cerr << object->ToString() << std::endl;
    ASSERT_EQ(old, object->ToString());
}

TEST(Relation, AddRelation) {
    auto len = ts::NewClass<ts::PrimitiveClass<double>>("len");
    auto point =
        ts::NewClass<ts::StructClass>("point", ts::NewClass<ts::PrimitiveClass<double>>("x"),
                                      ts::NewClass<ts::PrimitiveClass<double>>("y"));
    auto connected = ts::NewClass<ts::RelationClass>("connected", point, point);

    auto database = util::MakePtr<db::Database>(util::MakePtr<mem::File>("test.data"),
                                                db::OpenMode::kWrite, DEBUG_LOGGER);
    database->AddClass(point);
    database->AddClass(connected);

    database->AddNode(ts::New<ts::Struct>(point, 0.0, 1.0));
    database->AddNode(ts::New<ts::Struct>(point, 0.0, 0.0));
    database->AddNode(ts::New<ts::Relation>(connected, ID(1), ID(0)));
    database->AddNode(ts::New<ts::Relation>(connected, ID(0), ID(1)));

    database->PrintNodesIf(connected, db::kAll);
}

TEST(Relation, PatternMatchSimpleEdge) {
    auto point =
        ts::NewClass<ts::StructClass>("point", ts::NewClass<ts::PrimitiveClass<double>>("x"),
                                      ts::NewClass<ts::PrimitiveClass<double>>("y"));
    auto connected = ts::NewClass<ts::RelationClass>("connected", point, point);

    auto database = util::MakePtr<db::Database>(util::MakePtr<mem::File>("test.data"),
                                                db::OpenMode::kWrite, DEBUG_LOGGER);
    database->AddClass(point);
    database->AddClass(connected);

    database->AddNode(ts::New<ts::Struct>(point, 0.0, 1.0));
    database->AddNode(ts::New<ts::Struct>(point, 0.0, 0.0));
    database->AddNode(ts::New<ts::Relation>(connected, ID(1), ID(0)));
    database->AddNode(ts::New<ts::Relation>(connected, ID(0), ID(1)));

    auto pattern = util::MakePtr<db::Pattern>(point);
    pattern->AddRelation(connected, [](db::Node point_a, db::Node point_b) {
        return point_a.Data<ts::Struct>()->GetField<ts::Primitive<double>>("y")->Value() >
               point_b.Data<ts::Struct>()->GetField<ts::Primitive<double>>("y")->Value();
    });

    database->PrintNodesIf(connected, db::kAll);

    std::vector<ts::Struct::Ptr> result;
    database->PatternMatch(pattern, std::back_inserter(result));
    std::cerr << "RESULT" << std::endl;
    for (auto& structure : result) {
        std::cerr << structure->ToString() << std::endl;
    }
}

TEST(Relation, PatternMatchAngle) {
    auto point =
        ts::NewClass<ts::StructClass>("point", ts::NewClass<ts::PrimitiveClass<double>>("x"),
                                      ts::NewClass<ts::PrimitiveClass<double>>("y"));
    auto connected = ts::NewClass<ts::RelationClass>("connected", point, point);

    auto database = util::MakePtr<db::Database>(util::MakePtr<mem::File>("test.data"),
                                                db::OpenMode::kWrite, DEBUG_LOGGER);
    database->AddClass(point);
    database->AddClass(connected);

    database->AddNode(ts::New<ts::Struct>(point, 0.0, 1.0));
    database->AddNode(ts::New<ts::Struct>(point, 0.0, 0.0));
    database->AddNode(ts::New<ts::Struct>(point, 1.0, 0.0));
    database->AddNode(ts::New<ts::Struct>(point, -1.0, 0.0));
    database->AddNode(ts::New<ts::Struct>(point, 0.0, -1.0));

    database->AddNode(ts::New<ts::Relation>(connected, ID(1), ID(0)));
    database->AddNode(ts::New<ts::Relation>(connected, ID(1), ID(2)));
    database->AddNode(ts::New<ts::Relation>(connected, ID(1), ID(3)));
    database->AddNode(ts::New<ts::Relation>(connected, ID(1), ID(4)));

    auto pattern = util::MakePtr<db::Pattern>(point);
    pattern->AddRelation(connected, [](db::Node, db::Node) { return true; });
    pattern->AddRelation(connected, [](db::Node, db::Node) { return true; });

    database->PrintNodesIf(connected, db::kAll);

    std::vector<ts::Struct::Ptr> result;
    database->PatternMatch(pattern, std::back_inserter(result));
    std::cerr << "RESULT" << std::endl;
    for (auto& structure : result) {
        std::cerr << structure->ToString() << std::endl;
    }
}

TEST(Relation, PatternMatchColoredAngle) {
    auto point =
        ts::NewClass<ts::StructClass>("point", ts::NewClass<ts::PrimitiveClass<double>>("x"),
                                      ts::NewClass<ts::PrimitiveClass<double>>("y"));
    auto blue = ts::NewClass<ts::RelationClass>("blue", point, point);
    auto red = ts::NewClass<ts::RelationClass>("red", point, point);

    auto database = util::MakePtr<db::Database>(util::MakePtr<mem::File>("test.data"),
                                                db::OpenMode::kWrite, DEBUG_LOGGER);
    database->AddClass(point);
    database->AddClass(blue);
    database->AddClass(red);

    database->AddNode(ts::New<ts::Struct>(point, 0.0, 1.0));
    database->AddNode(ts::New<ts::Struct>(point, 0.0, 0.0));
    database->AddNode(ts::New<ts::Struct>(point, 1.0, 0.0));
    database->AddNode(ts::New<ts::Struct>(point, -1.0, 0.0));
    database->AddNode(ts::New<ts::Struct>(point, 0.0, -1.0));

    database->AddNode(ts::New<ts::Relation>(blue, ID(1), ID(0)));
    database->AddNode(ts::New<ts::Relation>(blue, ID(1), ID(2)));
    database->AddNode(ts::New<ts::Relation>(blue, ID(1), ID(3)));
    database->AddNode(ts::New<ts::Relation>(red, ID(1), ID(4)));

    auto pattern = util::MakePtr<db::Pattern>(point);
    pattern->AddRelation(blue, [](db::Node, db::Node) { return true; });
    pattern->AddRelation(red, [](db::Node, db::Node) { return true; });

    std::vector<ts::Struct::Ptr> result;
    database->PatternMatch(pattern, std::back_inserter(result));
    std::cerr << "RESULT" << std::endl;
    for (auto& structure : result) {
        std::cerr << structure->ToString() << std::endl;
    }
}

TEST(Relation, Star) {
    auto point = ts::NewClass<ts::PrimitiveClass<int>>("point");
    auto blue = ts::NewClass<ts::RelationClass>("blue", point, point);

    auto database = util::MakePtr<db::Database>(util::MakePtr<mem::File>("test.data"),
                                                db::OpenMode::kWrite, DEBUG_LOGGER);
    database->AddClass(point);
    database->AddClass(blue);

    for (auto i : std::views::iota(0, 70)) {
        database->AddNode(ts::New<ts::Primitive<int>>(point, i));
    }
    for (auto i : std::views::iota(0, 70)) {
        database->AddNode(ts::New<ts::Relation>(blue, ID(0), ID(i)));
        database->AddNode(ts::New<ts::Relation>(blue, ID(i), ID(0)));
    }
    auto star = util::MakePtr<db::Pattern>(point);
    for ([[maybe_unused]] auto i : std::views::iota(0, 10)) {
        star->AddRelation(blue, [](db::Node a, db::Node b) {
            return a.Data<ts::Primitive<int>>()->Value() == 0 &&
                   b.Data<ts::Primitive<int>>()->Value() >= 50;
        });
    }

    std::vector<ts::Struct::Ptr> result;
    database->PatternMatch(star, std::back_inserter(result));
    std::cerr << "RESULT" << std::endl;
    for (auto& structure : result) {
        std::cerr << structure->ToString() << std::endl;
    }
}

TEST(Relation, Stress) {
    auto point = ts::NewClass<ts::PrimitiveClass<int>>("point");
    auto blue = ts::NewClass<ts::RelationClass>("blue", point, point);
    auto red = ts::NewClass<ts::RelationClass>("red", point, point);

    auto database = util::MakePtr<db::Database>(util::MakePtr<mem::File>("test.data"),
                                                db::OpenMode::kWrite, DEBUG_LOGGER);
    database->AddClass(point);
    database->AddClass(blue);
    database->AddClass(red);

    for (auto i : std::views::iota(0, 60)) {
        database->AddNode(ts::New<ts::Primitive<int>>(point, i));
    }
    for (auto i : std::views::iota(0, 60)) {
        for (auto j : std::views::iota(0, i)) {
            auto color = rand() % 2 ? blue : red;
            database->AddNode(ts::New<ts::Relation>(color, ID(j), ID(i)));
            database->AddNode(ts::New<ts::Relation>(color, ID(i), ID(j)));
        }
    }
    auto edge = util::MakePtr<db::Pattern>(point);
    auto prev = edge;
    for ([[maybe_unused]] auto i : std::views::iota(0, 2)) {
        auto next = util::MakePtr<db::Pattern>(point);
        prev->AddRelation(blue, db::kAll, next);
        prev = next;
    }

    std::vector<ts::Struct::Ptr> result;
    database->PatternMatch(edge, std::back_inserter(result));
    std::cerr << "RESULT" << std::endl;
    for (auto& structure : result) {
        std::cerr << structure->ToString() << std::endl;
    }
}