# DaedalusDB

Graph Document-oriented Database Managment System that implements **ADIO** (All Data Is Object) concept.

*In development*

Current progress:
- 90% API
- 0% Query Language
- 0% Client-Server managment
- 0% Documentation TODO
- 10% Test Coverage - Smoke tests only


## API

**ADIO** means that all of data that your store in database is stored as *Objects*, so:

1. *Vertices* are *Objects* 
2. *Relations* are *Objects*
3. *Subgrapghs* are *Objects*


Every *Object* have *Class* that describes it's signature:

### Class declaration
```cpp
 auto person_class = ts::NewClass<ts::StructClass>(
        "person", ts::NewClass<ts::StringClass>("name"), 
        ts::NewClass<ts::StringClass>("surname"),
        ts::NewClass<ts::PrimitiveClass<int>>("age"),
        ts::NewClass<ts::PrimitiveClass<bool>>("male"));

auto coordinates_class = ts::NewClass<ts::StructClass>(
        "coordinates", ts::NewClass<ts::PrimitiveClass<double>>("lat"),
        ts::NewClass<ts::PrimitiveClass<double>>("lon"));

auto city_class = ts::NewClass<ts::StructClass>("city", 
            ts::NewClass<ts::StringClass>("name"), 
            coordinates_class);

```

### Object definition

```cpp

auto address_class = ts::NewClass<ts::StructClass>(
        "address", ts::NewClass<ts::StringClass>("city"),
        ts::NewClass<ts::StringClass>("street"),
        ts::NewClass<ts::PrimitiveClass<size_t>>("house"));
auto person_class = ts::NewClass<ts::StructClass>(
        "person", ts::NewClass<ts::StringClass>("name"),  
        ts::NewClass<ts::StringClass>("surname"),
        ts::NewClass<ts::PrimitiveClass<int>>("age"), address_class);

auto greg = ts::New<ts::Struct>(person_class, std::format("Greg {}",i),
"Sosnovtsev", 19, "Saint-Petersburg", "Nevskiy prospekt",i);
```



### Simple node addition 

```cpp
auto database = db::Database(util::MakePtr<mem::File>("perf.ddb"), db::OpenMode::kWrite);

auto name = ts::NewClass<ts::StringClass>("name");

database.AddClass(name);

database.AddNode(ts::New<ts::String>(name, "test name"));
```


### Simple relation addition

```cpp
auto point =
        ts::NewClass<ts::StructClass>("point",
        ts::NewClass<ts::PrimitiveClass<double>>("x"),
        ts::NewClass<ts::PrimitiveClass<double>>("y"));

auto connected = ts::NewClass<ts::RelationClass>("connected",point,point);

auto database = db::Database(util::MakePtr<mem::File("test.data"));
database.AddClass(point);
database.AddClass(connected);

database.AddNode(ts::New<ts::Struct>(point, 0.0, 1.0));
database.AddNode(ts::New<ts::Struct>(point, 0.0, 0.0));
database.AddNode(ts::New<ts::Relation>(connected, ID(1), ID(0)));
database.AddNode(ts::New<ts::Relation>(connected, ID(0), ID(1)));
```

### Pattern Matching

```cpp
auto pattern = util::MakePtr<db::Pattern>(point);
pattern->AddRelation(connected, [](db::Node point_a, db::Node point_b) {
        return point_a.Data<ts::Struct>()->GetField<ts::Primitive<double>>("y")->Value() >
               point_b.Data<ts::Struct>()->GetField<ts::Primitive<double>>("y")->Value();
});
std::vector<ts::Struct::Ptr> result;
database.PatternMatch(pattern, std::back_inserter(result));
```

For more examples you can check tests folder, there are some smoke tests which I made during development.


## Performance

## Insertion

Insertion of simple elements.

![Insert plot](./tests/test_results/Insert.png) 

Time complexity: **O(1)**

## Deletion

Removal of valued size and variable sized objects into database. The Logic of Iterators are little bit different and Valued sized Objects Storage had some optimizations hence the performance could be different.

![RemoveVal plot](./tests/test_results/RemoveVal.png) 

![RemoveVar plot](./tests/test_results/RemoveVar.png) 

Time complexity: **O(|A|)** where A is set of elements of certain class.


## Compression

The database some primitive file compression mechanism implemented so filesize is linear to the overall number of elements.

![RemoveVar plot](./tests/test_results/Compression.png) 

## Match

Here is some performance test for Pattern Matching of simple pattern (see *performance_test.cpp*). Time complexity analysis is pretty complex so a left it for future, but it definitely depends on number of verticies and edges. 

![RemoveVar plot](./tests/test_results/Match.png) 

___


If you want to contribute you are welcome :)