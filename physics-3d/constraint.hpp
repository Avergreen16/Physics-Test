#pragma once

namespace axiom {

struct constraint {
    virtual void before() = 0;
    virtual void solve(float delta_time) = 0;
    virtual void after() = 0;
};

}