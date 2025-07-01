#ifndef UTIL_POLY_H
#define UTIL_POLY_H

#include <algorithm>
#include <cmath>
#include <cfloat>
#include <limits>
#include <optional>
#include <stack>

#include <util/circular_buffer.h>

namespace util
{
    /* ------------------------------------------------------------------------- */
    /*                           PLAYER‑INTERSECTION                             */
    /* ------------------------------------------------------------------------- */
    struct player_intersection
    {
        inline static constexpr auto sphere_count = 10;

        struct
        {
            bool                                    valid{};
            sdk::cs_player_t::hitbox                index{};
            int32_t                                 group{};
            float                                   radius{ -1.f };
            std::array<std::array<sdk::vec3, sphere_count>, 3> spheres{};
        } hitboxes[static_cast<int32_t>(sdk::cs_player_t::hitbox::max)]{};

        uint8_t cnt{};

        player_intersection(const sdk::studiohdr* hdr,
            int32_t                           set,
            std::array<sdk::mat3x4*, 3>& mat)
        {
            cnt = !!mat[0] + !!mat[1] + !!mat[2];

            for (const auto& hb : sdk::cs_player_t::hitboxes)
            {
                auto& box = hitboxes[static_cast<int32_t>(hb)];
                box.valid = false;

                const auto hitbox = hdr->get_hitbox(static_cast<int32_t>(hb), set);
                if (!hitbox || hitbox->radius <= 0.f)
                    continue;

                box = { true, hb, hitbox->group, hitbox->radius, {} };

                for (auto j = 0; j < cnt; ++j)
                {
                    auto& spheres = box.spheres[j];
                    auto  num_spheres = 0;

                    vector_transform(hitbox->bbmax, mat[j][hitbox->bone], spheres[num_spheres++]);
                    vector_transform(hitbox->bbmin, mat[j][hitbox->bone], spheres[num_spheres++]);

                    const auto delta = (spheres[0] - spheres[1]) /
                        static_cast<float>(sphere_count - 1);
                    for (auto p = 0; p < sphere_count - 2; ++p, ++num_spheres)
                        spheres[num_spheres] = spheres[num_spheres - 1] + delta;
                }
            }
        }

        /* --------------------------------------------------------------------- */
        /*                          HITGROUP‑TRACE                               */
        /* --------------------------------------------------------------------- */
        inline int32_t trace_hitgroup(const sdk::ray& r, bool scan_secure) const
        {
            auto        res = hitgroup_gear;
            bool        hit_head = false;
            float       head_frac = FLT_MAX;
            const auto  dir = sdk::vec3(r.delta).normalize();

            for (const auto& hb : sdk::cs_player_t::hitboxes)
            {
                const auto& box = hitboxes[static_cast<int32_t>(hb)];
                if (!box.valid)
                    continue;

                for (auto i = 0; i < cnt; ++i)
                {
                    if (!scan_secure && i > 0)
                        break;

                    for (auto s = 0; s < sphere_count; ++s)
                    {
                        if (box.group == hitgroup_head ||
                            box.group == hitgroup_chest ||
                            box.group == hitgroup_stomach)
                        {
                            const auto frac =
                                intersect_sphere(r, dir, box.spheres[i][s], box.radius);
                            if (frac == FLT_MAX)
                                continue;

                            if (box.group == hitgroup_head)
                            {
                                hit_head = true;
                                if (frac < head_frac)
                                    head_frac = frac;
                            }
                            else if (hit_head && frac < head_frac + .1f)
                                hit_head = false;

                            if (box.group != hitgroup_head)
                                res = best_hg(box.group, res);
                        }
                        else if (intersect_sphere_approx(r, dir, box.spheres[i][s], box.radius))
                            res = best_hg(box.group, res);
                    }
                }
            }

            if (hit_head)
                res = hitgroup_head;

            return res;
        }

        __forceinline int32_t rank(int32_t i) const
        {
            switch (i)
            {
            case hitgroup_head:     
                return 1;
            case hitgroup_stomach:  
                return 2;
            case hitgroup_chest:    
                return 3;
            case hitgroup_leftarm:
            case hitgroup_rightarm: 
                return 4;
            case hitgroup_leftleg:
            case hitgroup_rightleg: 
                return 5;
            default:
                return 6;
            }
        }

        inline int32_t best_hg(int32_t n, int32_t o) const
        {
            return rank(n) < rank(o) ? n : o;
        }

        inline bool intersect_sphere_approx(const sdk::ray& r,
            const sdk::vec3& dir,
            const sdk::vec3& sphere,
            float            radius) const
        {
            const auto q = sphere - r.start;
            const auto v = q.dot(dir);
            const auto dist_sq = q.length_sqr() - v * v;
            return (radius * radius) >= dist_sq;
        }

        inline float intersect_sphere(const sdk::ray& r,
            const sdk::vec3& dir,
            const sdk::vec3& sphere,
            float            radius) const
        {
            const auto q = r.start - sphere;
            const auto v = q.dot(dir);
            const auto w = q.dot(q) - radius * radius;

            if (w > 0.f && v > 0.f)
                return FLT_MAX;

            const auto discr = v * v - w;
            if (discr < 0.f)
                return FLT_MAX;

            const auto t = -v - std::sqrt(discr);
            return t < 0.f ? 0.f : t;
        }
    };

    /* ------------------------------------------------------------------------- */
    /*                         GEOMETRY‑/POLY UTILITIES                          */
    /* ------------------------------------------------------------------------- */
    using convex_polygon = circular_buffer<sdk::vec3>;

    inline bool is_equal(float d1, float d2)
    {
        constexpr auto tol = .001f;
        return std::fabs(d1 - d2) <= tol;
    }

    [[nodiscard]] inline bool strict_less(const sdk::vec3& a, const sdk::vec3& b) noexcept
    {
        /*  We must provide a **strict‑weak‑ordering**.  Tolerance‑based “fuzzy”
            comparisons (like the previous is_equal‑mix) can violate asymmetry
            (cmp(a,b) && cmp(b,a)) and trigger the STL “invalid comparator”
            debug assertion.                                                        */
        if (a.x < b.x)
            return true;

        if (a.x > b.x)
            return false;

        return a.y < b.y;                           // x are equal → tie‑break on y
    }

    /*  Point‑in‑polygon (ray‑casting, even‑odd rule) – polygon is assumed convex */
    inline bool inside_poly(const sdk::vec3& test, const convex_polygon& poly)
    {
        bool res = false;
        for (std::size_t i = 0, j = poly.size() - 1; i < poly.size(); j = i++)
        {
            if ((poly[i].y > test.y) != (poly[j].y > test.y) &&
                (test.x <
                    (poly[j].x - poly[i].x) * (test.y - poly[i].y) /
                    (poly[j].y - poly[i].y) +
                    poly[i].x))
            {
                res = !res;
            }
        }
        return res;
    }

    /*  Precise line‑segment intersection – returns nullopt when segments do not
        intersect on their finite spans */
    inline std::optional<sdk::vec3> get_intersection(const sdk::vec3& l1p1,
        const sdk::vec3& l1p2,
        const sdk::vec3& l2p1,
        const sdk::vec3& l2p2)
    {
        const auto a1 = l1p2.y - l1p1.y;
        const auto b1 = l1p1.x - l1p2.x;
        const auto c1 = a1 * l1p1.x + b1 * l1p1.y;

        const auto a2 = l2p2.y - l2p1.y;
        const auto b2 = l2p1.x - l2p2.x;
        const auto c2 = a2 * l2p1.x + b2 * l2p1.y;

        const auto det = a1 * b2 - a2 * b1;
        if (is_equal(det, 0.f))
            return {};

        const auto x = (b2 * c1 - b1 * c2) / det;
        const auto y = (a1 * c2 - a2 * c1) / det;

        const auto on_line1 = ((min(l1p1.x, l1p2.x) < x || is_equal(min(l1p1.x, l1p2.x), x)) &&
            (max(l1p1.x, l1p2.x) > x || is_equal(max(l1p1.x, l1p2.x), x)) &&
            (min(l1p1.y, l1p2.y) < y || is_equal(min(l1p1.y, l1p2.y), y)) &&
            (max(l1p1.y, l1p2.y) > y || is_equal(max(l1p1.y, l1p2.y), y)));

        const auto on_line2 = ((min(l2p1.x, l2p2.x) < x || is_equal(min(l2p1.x, l2p2.x), x)) &&
            (max(l2p1.x, l2p2.x) > x || is_equal(max(l2p1.x, l2p2.x), x)) &&
            (min(l2p1.y, l2p2.y) < y || is_equal(min(l2p1.y, l2p2.y), y)) &&
            (max(l2p1.y, l2p2.y) > y || is_equal(max(l2p1.y, l2p2.y), y)));

        return (on_line1 && on_line2) ? std::optional<sdk::vec3>{ sdk::vec3(x, y, 0.f) } : std::nullopt;
    }

    /*  Deduplicated point insertion into circular_buffer */
    inline void add_points(convex_polygon& points, const sdk::vec3& np)
    {
        bool found = false;
        for (std::size_t j = 0; j < points.size(); ++j)
        {
            const auto& p = points[j];
            if (is_equal(p.x, np.x) && is_equal(p.y, np.y))
            {
                found = true;
                break;
            }
        }

        if (!found && !points.exhausted())
            *points.push_front() = np;
    }

    /*  Centres & sorts polygon vertices in clockwise order                     */
    inline void order_clockwise(convex_polygon& points)
    {
        float mx = 0.f, my = 0.f;

        for (std::size_t i = 0; i < points.size(); ++i)
        {
            mx += points[i].x;
            my += points[i].y;
        }
        mx /= points.size();
        my /= points.size();

        for (std::size_t i = 0; i < points.size(); ++i)
            points[i].z = std::atan2(points[i].y - my, points[i].x - mx);

        points.sort([](const sdk::vec3& a, const sdk::vec3& b) { return a.z > b.z; });
    }

    /*  Signed area (shoelace)                                                  */
    inline float signed_area(const convex_polygon& poly)
    {
        float a = 0.f;
        std::size_t j = poly.size() - 1;
        for (std::size_t i = 0; i < poly.size(); ++i)
        {
            a += (poly[j].x + poly[i].x) * (poly[j].y - poly[i].y);
            j = i;
        }
        return a * 0.5f;
    }

    inline float area(const convex_polygon& poly) { return std::fabs(signed_area(poly)); }

    /* ------------------------------------------------------------------------- */
    /*                SUTHERLAND–HODGMAN POLYGON CLIPPING                       */
    /* ------------------------------------------------------------------------- */
    inline convex_polygon get_intersection_poly(const convex_polygon& poly1,
        const convex_polygon& poly2)
    {
        /* Guard against degenerate input                                         */
        if (poly1.size() < 3 || poly2.size() < 3)
            return {};

        /* Convert circular_buffers to contiguous vectors for easier processing   */
        std::vector<sdk::vec3> subject;
        subject.reserve(poly1.size());
        for (std::size_t i = 0; i < poly1.size(); ++i)
            subject.push_back(poly1[i]);

        std::vector<sdk::vec3> clip;
        clip.reserve(poly2.size());
        for (std::size_t i = 0; i < poly2.size(); ++i)
            clip.push_back(poly2[i]);

        /* Determine orientation of the clip‑polygon (needed for half‑plane test) */
        const bool clip_clockwise = (signed_area(poly2) < 0.f);

        auto inside = [clip_clockwise](const sdk::vec3& p,
            const sdk::vec3& a,
            const sdk::vec3& b) -> bool
            {
                const float cross =
                    (b.x - a.x) * (p.y - a.y) - (b.y - a.y) * (p.x - a.x);
                return clip_clockwise ? (cross <= 0.f) : (cross >= 0.f);
            };

        auto compute_intersection = [](const sdk::vec3& s,
            const sdk::vec3& e,
            const sdk::vec3& a,
            const sdk::vec3& b) -> sdk::vec3
            {
                const float a1 = e.y - s.y;
                const float b1 = s.x - e.x;
                const float c1 = a1 * s.x + b1 * s.y;

                const float a2 = b.y - a.y;
                const float b2 = a.x - b.x;
                const float c2 = a2 * a.x + b2 * a.y;

                const float det = a1 * b2 - a2 * b1;
                if (std::fabs(det) < 1e-6f)
                    return { std::numeric_limits<float>::quiet_NaN(),
                             std::numeric_limits<float>::quiet_NaN(),
                             0.f };

                const float x = (b2 * c1 - b1 * c2) / det;
                const float y = (a1 * c2 - a2 * c1) / det;
                return { x, y, 0.f };
            };

        /* Core S–H loop – iterate over each edge of the clip polygon             */
        for (std::size_t i = 0; i < clip.size(); ++i)
        {
            const std::size_t next = (i + 1 == clip.size()) ? 0 : i + 1;
            const sdk::vec3   cp1 = clip[i];
            const sdk::vec3   cp2 = clip[next];

            if (subject.empty())
                break;

            std::vector<sdk::vec3> output;
            output.reserve(subject.size());

            sdk::vec3 S = subject.back();

            for (const auto& E : subject)
            {
                const bool E_in = inside(E, cp1, cp2);
                const bool S_in = inside(S, cp1, cp2);

                if (E_in)
                {
                    if (!S_in)
                        output.push_back(compute_intersection(S, E, cp1, cp2));
                    output.push_back(E);
                }
                else if (S_in)
                {
                    output.push_back(compute_intersection(S, E, cp1, cp2));
                }
                S = E;
            }
            subject.swap(output);
        }

        if (subject.size() < 3)
            return {};

        /* Move result into a convex_polygon (already in perimeter order)         */
        convex_polygon result(subject.size());
        result.resize(subject.size());
        for (std::size_t i = 0; i < subject.size(); ++i)
            result[i] = subject[i];

        /* No need to reorder – S‑H output is already ordered.                    */
        return result;
    }

    /* ------------------------------------------------------------------------- */
    /*                   MONOTONE‑CHAIN (Andrew’s) CONVEX HULL                  */
    /* ------------------------------------------------------------------------- */
    inline float cross(const sdk::vec3& O, const sdk::vec3& A, const sdk::vec3& B)
    {
        return (A.x - O.x) * (B.y - O.y) - (A.y - O.y) * (B.x - O.x);
    }

    inline void monotone_chain(convex_polygon& points)
    {
        const std::size_t n = points.size();
        if (n <= 3)
            return;

        /* Copy points to a contiguous container and sort strictly lexicographic  */
        std::vector<sdk::vec3> v;
        v.reserve(n);
        for (std::size_t i = 0; i < n; ++i)
            v.push_back(points[i]);

        std::sort(v.begin(), v.end(), strict_less);      // ←‑‑ FIX: safe comparator

        /* Build lower + upper hull                                               */
        std::vector<sdk::vec3> hull(2 * n);
        std::size_t            k = 0;

        for (const auto& p : v)                         // lower hull
        {
            while (k >= 2 && cross(hull[k - 2], hull[k - 1], p) <= 0.f)
                --k;
            hull[k++] = p;
        }
        for (std::size_t i = v.size() - 1, t = k + 1; i-- > 0;) // upper hull
        {
            while (k >= t && cross(hull[k - 2], hull[k - 1], v[i]) <= 0.f)
                --k;
            hull[k++] = v[i];
        }

        if (k > 1) --k;                                 // last point duplicates first

        /* Export back to circular_buffer                                         */
        convex_polygon out(k);
        out.resize(k);
        for (std::size_t i = 0; i < k; ++i)
            out[i] = hull[i];

        points = std::move(out);
    }

} // namespace util

#endif /* UTIL_POLY_H */
