// Copyright (c) 2010-2023, Lawrence Livermore National Security, LLC. Produced
// at the Lawrence Livermore National Laboratory. All Rights reserved. See files
// LICENSE and NOTICE for details. LLNL-CODE-806117.
//
// This file is part of the MFEM library. For more information and source code
// availability visit https://mfem.org.
//
// MFEM is free software; you can redistribute it and/or modify it under the
// terms of the BSD-3 license. We welcome feedback and contributions, see file
// CONTRIBUTING.md for details.

#include "mfem.hpp"
#include "unit_tests.hpp"

using namespace mfem;

void FillArraysByName(ArraysByName<int> &abn)
{
   abn.CreateArray("1 potato");
   abn.CreateArray("2 potato");
   abn.CreateArray("3 potato");
   abn.CreateArray("four");

   Array<int> a3({2.0, 4.0, 6.0});
   int ContigData[4] = {4, 3, 2, 1};
   Array<int> a4(ContigData, 4);

   abn["1 potato"].SetSize(1); abn["1 potato"][0] = 100;
   abn["2 potato"].Append(5);  abn["2 potato"].Append(10);
   abn["3 potato"] = a3;
   abn["four"] = a4;
}

TEST_CASE("ArraysByName range-based for loop", "[ArraysByName]")
{
   ArraysByName<int> abn;

   FillArraysByName(abn);

   // Test standard Iterator
   int i = 1;
   for (auto a : abn)
   {
      REQUIRE(a.second.Size() == i);
      i++;
   }

   // Test ConstIterator
   const ArraysByName<int> &abnc = abn;
   i = 1;
   for (auto a : abnc)
   {
      REQUIRE(a.second.Size() == i);
      i++;
   }
}

TEST_CASE("ArraysByName Copy Methods", "[ArraysByName]")
{
   ArraysByName<int> abn;

   FillArraysByName(abn);

   // Explicit call to copy constructor
   ArraysByName<int> abn_copy1(abn);
   REQUIRE(abn == abn_copy1);

   // Implicit call to copy constructor
   ArraysByName<int> abn_copy2 = abn;
   REQUIRE(abn == abn_copy2);

   // Copy assignment operator
   ArraysByName<int> abn_copy3;
   abn_copy3 = abn;
   REQUIRE(abn == abn_copy3);

   // Explicit call to move constructor
   ArraysByName<int> abn_copy4a(abn);
   ArraysByName<int> abn_copy4b(std::move(abn_copy4a));
   REQUIRE(abn == abn_copy4b);

   // Implicit call to move constructor
   ArraysByName<int> abn_copy5a(abn);
   ArraysByName<int> abn_copy5b = std::move(abn_copy5a);
   REQUIRE(abn == abn_copy5b);

   // Move assignment operator
   ArraysByName<int> abn_copy6a(abn);
   ArraysByName<int> abn_copy6b;
   abn_copy6b = std::move(abn_copy6a);
   REQUIRE(abn == abn_copy6b);

   // Implicit call to convertible type copy constructor
   ArraysByName<long> abn_copy7 = abn;
   REQUIRE(abn.Size() == abn_copy7.Size());
   for (auto entry7 : abn_copy7)
   {
      std::string name = entry7.first;
      REQUIRE(abn.EntryExists(name));
      REQUIRE(abn[name].Size() == abn_copy7[name].Size());
      int matches = 0;
      for (int i=0; i<abn[name].Size(); i++)
      {
         if ((long)abn[name][i] == abn_copy7[name][i]) { matches++; }
      }
      REQUIRE(matches == abn[name].Size());
   }

   // Convertible type copy assignment operator
   ArraysByName<long> abn_copy8;
   abn_copy8 = abn;
   REQUIRE(abn.Size() == abn_copy8.Size());
   for (auto entry8 : abn_copy8)
   {
      std::string name = entry8.first;
      REQUIRE(abn.EntryExists(name));
      REQUIRE(abn[name].Size() == abn_copy8[name].Size());
      int matches = 0;
      for (int i=0; i<abn[name].Size(); i++)
      {
         if ((long)abn[name][i] == abn_copy8[name][i]) { matches++; }
      }
      REQUIRE(matches == abn[name].Size());
   }
}

TEST_CASE("ArraysByName Various Methods", "[ArraysByName]")
{
   ArraysByName<int> abn;

   FillArraysByName(abn);

   // Compare the two GetNames methods
   std::set<std::string> names1; abn.GetNames(names1);
   std::set<std::string> names2 = abn.GetNames();
   REQUIRE(names1.size() == 4);
   REQUIRE(names1.size() == names2.size());

   for (auto it1 = names1.begin(), it2 = names2.begin();
        it1 != names1.end() && it2 != names2.end(); it1++, it2++)
   {
      REQUIRE(*it1 == *it2);
   }

   // Get set names and verify that they are valid
   std::set<std::string> names = abn.GetNames();
   REQUIRE(abn.Size() == (int)names.size());

   for (auto name : names)
   {
      REQUIRE(abn.EntryExists(name));
   }

   // Check for existence (or not) of specific named sets
   REQUIRE(abn.EntryExists("1 potato"));
   REQUIRE(abn.EntryExists("2 potato"));
   REQUIRE(abn.EntryExists("3 potato"));
   REQUIRE(abn.EntryExists("four"));
   REQUIRE(!abn.EntryExists("5 potato"));

   abn.CreateArray("5 potato");
   REQUIRE(abn.EntryExists("5 potato"));

   abn.DeleteArray("5 potato");
   REQUIRE(!abn.EntryExists("5 potato"));

   abn.DeleteAll();
   REQUIRE(!abn.EntryExists("1 potato"));
   REQUIRE(!abn.EntryExists("2 potato"));
   REQUIRE(!abn.EntryExists("3 potato"));
   REQUIRE(!abn.EntryExists("four"));
   REQUIRE(!abn.EntryExists("5 potato"));
   REQUIRE(abn.Size() == 0);
}
