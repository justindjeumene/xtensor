/***************************************************************************
* Copyright (c) 2016, Johan Mabille, Sylvain Corlay and Wolf Vollprecht    *
*                                                                          *
* Distributed under the terms of the BSD 3-Clause License.                 *
*                                                                          *
* The full license is in the file LICENSE, distributed with this software. *
****************************************************************************/

#include "gtest/gtest.h"
#include "xtensor/xarray.hpp"
#include "test_common.hpp"

namespace xt
{
    using std::size_t;

    struct xfunction_features
    {
        xarray<int, layout_type::dynamic> m_a;  // shape = { 3, 2, 4 }
        xarray<int, layout_type::dynamic> m_b;  // shape = { 3, 1, 4 }
        xarray<int, layout_type::dynamic> m_c;  // shape = { 4, 3, 2, 4 }

        xfunction_features();
    };

    xfunction_features::xfunction_features()
    {
        row_major_result<> rm;
        m_a.reshape(rm.shape(), rm.strides());
        std::copy(rm.data().cbegin(), rm.data().cend(), m_a.begin());

        unit_shape_result<> us;
        m_b.reshape(us.shape(), us.strides());
        std::copy(us.data().cbegin(), us.data().cend(), m_b.begin());

        using shape_type = layout_result<>::shape_type;
        shape_type sh = {4, 3, 2, 4};
        m_c.reshape(sh);

        for (size_t i = 0; i < sh[0]; ++i)
        {
            for (size_t j = 0; j < sh[1]; ++j)
            {
                for (size_t k = 0; k < sh[2]; ++k)
                {
                    for (size_t l = 0; l < sh[3]; ++l)
                    {
                        m_c(i, j, k, l) = m_a(j, k, l) + static_cast<int>(i);
                    }
                }
            }
        }
    }

    TEST(xfunction, broadcast_shape)
    {
        using shape_type = layout_result<>::shape_type;
        xfunction_features f;

        {
            SCOPED_TRACE("same shape");
            shape_type sh(3, size_t(1));
            bool trivial = (f.m_a + f.m_a).broadcast_shape(sh);
            EXPECT_EQ(sh, f.m_a.shape());
            ASSERT_TRUE(trivial);
        }

        {
            SCOPED_TRACE("different shape");
            shape_type sh(3, size_t(1));
            bool trivial = (f.m_a + f.m_b).broadcast_shape(sh);
            EXPECT_EQ(sh, f.m_a.shape());
            ASSERT_FALSE(trivial);
        }

        {
            SCOPED_TRACE("different dimensions");
            shape_type sh(4, size_t(1));
            bool trivial = (f.m_a + f.m_c).broadcast_shape(sh);
            EXPECT_EQ(sh, f.m_c.shape());
            ASSERT_FALSE(trivial);
        }
    }

    TEST(xfunction, layout_type)
    {
        xarray<int, layout_type::dynamic> m_d;
        xarray<int, layout_type::row_major> m_r;
        xarray<int, layout_type::column_major> m_c;
        xscalar<int> m_a(2);

        auto res_dd = m_d + m_d;
        EXPECT_EQ(res_dd.layout(), layout_type::dynamic);
        auto res_dr = m_d + m_r;
        EXPECT_EQ(res_dr.layout(), layout_type::dynamic);
        auto res_dc = m_d + m_c;
        EXPECT_EQ(res_dc.layout(), layout_type::dynamic);
        auto res_da = m_d + m_a;
        EXPECT_EQ(res_da.layout(), layout_type::dynamic);

        auto res_rd = m_r + m_d;
        EXPECT_EQ(res_rd.layout(), layout_type::dynamic);
        auto res_rr = m_r + m_r;
        EXPECT_EQ(res_rr.layout(), layout_type::row_major);
        auto res_rc = m_r + m_c;
        EXPECT_EQ(res_rc.layout(), layout_type::dynamic);
        auto res_ra = m_r + m_a;
        EXPECT_EQ(res_ra.layout(), layout_type::row_major);

        auto res_cd = m_c + m_d;
        EXPECT_EQ(res_cd.layout(), layout_type::dynamic);
        auto res_cr = m_c + m_r;
        EXPECT_EQ(res_cr.layout(), layout_type::dynamic);
        auto res_cc = m_c + m_c;
        EXPECT_EQ(res_cc.layout(), layout_type::column_major);
        auto res_ca = m_c + m_a;
        EXPECT_EQ(res_ca.layout(), layout_type::column_major);

        auto res_ad = m_a + m_d;
        EXPECT_EQ(res_ad.layout(), layout_type::dynamic);
        auto res_ar = m_a + m_r;
        EXPECT_EQ(res_ar.layout(), layout_type::row_major);
        auto res_ac = m_a + m_c;
        EXPECT_EQ(res_ac.layout(), layout_type::column_major);
        auto res_aa = m_a + m_a;
        EXPECT_EQ(res_aa.layout(), layout_type::any);
    }

    TEST(xfunction, access)
    {
        xfunction_features f;
        size_t i = f.m_a.shape()[0] - 1;
        size_t j = f.m_a.shape()[1] - 1;
        size_t k = f.m_a.shape()[2] - 1;

        {
            SCOPED_TRACE("same shape");
            int a = (f.m_a + f.m_a)(i, j, k);
            int b = f.m_a(i, j, k) + f.m_a(i, j, k);
            EXPECT_EQ(a, b);
        }

        {
            SCOPED_TRACE("different shape");
            int a = (f.m_a + f.m_b)(i, j, k);
            int b = f.m_a(i, j, k) + f.m_b(i, 0, k);
            EXPECT_EQ(a, b);
        }

        {
            SCOPED_TRACE("different dimensions");
            int a = (f.m_a + f.m_c)(1, i, j, k);
            int b = f.m_a(i, j, k) + f.m_c(1, i, j, k);
            EXPECT_EQ(a, b);
        }
    }

    TEST(xfunction, at)
    {
        xfunction_features f;
        size_t i = f.m_a.shape()[0] - 1;
        size_t j = f.m_a.shape()[1] - 1;
        size_t k = f.m_a.shape()[2] - 1;

        {
            SCOPED_TRACE("same shape");
            int a = (f.m_a + f.m_a).at(i, j, k);
            int b = f.m_a.at(i, j, k) + f.m_a.at(i, j, k);
            EXPECT_EQ(a, b);
            EXPECT_ANY_THROW((f.m_a + f.m_a).at(0, 0, 0, 0));
            EXPECT_ANY_THROW((f.m_a + f.m_a).at(10, 10, 10));
        }

        {
            SCOPED_TRACE("different shape");
            int a = (f.m_a + f.m_b).at(i, j, k);
            int b = f.m_a.at(i, j, k) + f.m_b.at(i, 0, k);
            EXPECT_EQ(a, b);
            EXPECT_ANY_THROW((f.m_a + f.m_a).at(0, 0, 0, 0));
            EXPECT_ANY_THROW((f.m_a + f.m_a).at(10, 10, 10));
        }

        {
            SCOPED_TRACE("different dimensions");
            int a = (f.m_a + f.m_c).at(1, i, j, k);
            int b = f.m_a.at(i, j, k) + f.m_c.at(1, i, j, k);
            EXPECT_EQ(a, b);
            EXPECT_ANY_THROW((f.m_a + f.m_a).at(0, 0, 0, 0, 0));
            EXPECT_ANY_THROW((f.m_a + f.m_a).at(10, 10, 10, 10));
        }
    }

    TEST(xfunction, indexed_access)
    {
        xfunction_features f;
        xindex index(f.m_a.dimension());
        index[0] = f.m_a.shape()[0] - 1;
        index[1] = f.m_a.shape()[1] - 1;
        index[2] = f.m_a.shape()[2] - 1;

        {
            SCOPED_TRACE("same shape");
            int a = (f.m_a + f.m_a)[index];
            int b = f.m_a[index] + f.m_a[index];
            EXPECT_EQ(a, b);
            EXPECT_EQ(((f.m_a + f.m_a)[{0, 0, 0}]), (f.m_a[{0, 0, 0}] + f.m_a[{0, 0, 0}]));
        }

        {
            SCOPED_TRACE("different shape");
            int a = (f.m_a + f.m_b)[index];
            xindex index2 = index;
            index2[1] = 0;
            int b = f.m_a[index] + f.m_b[index2];
            EXPECT_EQ(a, b);
            EXPECT_EQ(((f.m_a + f.m_b)[{0, 0, 0}]), (f.m_a[{0, 0, 0}] + f.m_b[{0, 0, 0}]));
        }

        {
            SCOPED_TRACE("different dimensions");
            xindex index2(f.m_c.dimension());
            index2[0] = 1;
            index2[1] = index[0];
            index2[2] = index[1];
            index2[3] = index[2];
            int a = (f.m_a + f.m_c)[index2];
            int b = f.m_a[index] + f.m_c[index2];
            EXPECT_EQ(a, b);
            EXPECT_EQ(((f.m_a + f.m_c)[{0, 0, 0, 0}]), (f.m_a[{0, 0, 0, 0}] + f.m_c[{0, 0, 0, 0}]));
        }
    }

    void test_xfunction_iterator(const xarray<int>& a, const xarray<int>& b)
    {
        auto func = (a + b);
        auto iter = func.begin();
        auto itera = a.begin();
        auto iterb = b.begin(a.shape());
        auto nb_iter = a.shape().back() * 2 + 1;
        for (size_t i = 0; i < nb_iter; ++i)
        {
            ++iter, ++itera, ++iterb;
        }
        EXPECT_EQ(*iter, *itera + *iterb);
    }

    TEST(xfunction, iterator)
    {
        xfunction_features f;

        {
            SCOPED_TRACE("same shape");
            test_xfunction_iterator(f.m_a, f.m_a);
        }

        {
            SCOPED_TRACE("different shape");
            test_xfunction_iterator(f.m_a, f.m_b);
        }

        {
            SCOPED_TRACE("different dimensions");
            test_xfunction_iterator(f.m_c, f.m_a);
        }
    }

    void test_xfunction_iterator_end(const xarray<int>& a, const xarray<int>& b)
    {
        auto func = (a + b);
        auto iter = func.begin();
        auto iter_end = func.end();
        auto size = a.size();
        for (size_t i = 0; i < size; ++i)
        {
            ++iter;
        }
        EXPECT_EQ(iter, iter_end);
    }

    TEST(xfunction, iterator_end)
    {
        xfunction_features f;

        {
            SCOPED_TRACE("same shape");
            test_xfunction_iterator_end(f.m_a, f.m_a);
        }

        {
            SCOPED_TRACE("different shape");
            test_xfunction_iterator_end(f.m_a, f.m_b);
        }

        {
            SCOPED_TRACE("different dimensions");
            test_xfunction_iterator_end(f.m_c, f.m_a);
        }
    }
}
