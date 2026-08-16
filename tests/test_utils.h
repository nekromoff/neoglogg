/*
 * Copyright (c) 2026+ Daniel Duris, dusoft@staznosti.sk
 * Copyright (c) 2009–2018 Nicolas Bonnefon and other contributors
 *
 * This file is part of neoglogg.
 *
 * neoglogg is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * neoglogg is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with neoglogg.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef TEST_UTILS_H
#define TEST_UTILS_H

#include "gmock/gmock.h"

#include <string>
#include <chrono>
struct TestTimer {
    TestTimer()
        : TestTimer(
                ::testing::UnitTest::GetInstance()->current_test_info()->test_case_name() ) {
    text_ += std::string {"."} + std::string {::testing::UnitTest::GetInstance()->current_test_info()->name() };
    }

    TestTimer(const std::string& text)
        : Start { std::chrono::system_clock::now() }
        , text_ {text} {}

    virtual ~TestTimer() {
        using namespace std;
        Stop = chrono::system_clock::now();
        Elapsed = chrono::duration_cast<chrono::microseconds>(Stop - Start);
        cout << endl << text_ << " elapsed time = "
            << Elapsed.count() * 0.001 << "ms" << endl;
    }

    std::chrono::time_point<std::chrono::system_clock> Start;
    std::chrono::time_point<std::chrono::system_clock> Stop;
    std::chrono::microseconds Elapsed;
    std::string text_;
};

class SafeQSignalSpy : public QSignalSpy {
  public:
    template <typename... Args>
    SafeQSignalSpy( Args&&... args )
        : QSignalSpy( std::forward<Args>(args)... ) {}

    bool safeWait( int timeout = 10000 ) {
        // If it has already been received
        bool result = count() > 0;
        if ( ! result ) {
            result = wait( timeout );
        }
        return result;
    }
};

#endif
