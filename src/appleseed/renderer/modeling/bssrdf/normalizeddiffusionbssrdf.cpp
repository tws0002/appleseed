
//
// This source file is part of appleseed.
// Visit http://appleseedhq.net/ for additional information and resources.
//
// This software is released under the MIT license.
//
// Copyright (c) 2015 Esteban Tovagliari, The appleseedhq Organization
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
// THE SOFTWARE.
//

// Interface header.
#include "normalizeddiffusionbssrdf.h"

// appleseed.renderer headers.
#include "renderer/kernel/shading/shadingpoint.h"
#include "renderer/modeling/input/inputevaluator.h"

// appleseed.foundation headers.
#include "foundation/math/scalar.h"
#include "foundation/math/sss.h"
#include "foundation/utility/containers/dictionary.h"
#include "foundation/utility/containers/specializedarrays.h"

using namespace foundation;

namespace renderer
{

namespace
{

    const char* Model = "normalized_diffusion_bssrdf";

    //
    // Normalized diffusion BSSRDF.
    //
    // Reference:
    //
    //   Approximate Reflectance Profiles for Efficient Subsurface Scattering
    //   Per H. Christensen, Brent Burley
    //   http://graphics.pixar.com/library/ApproxBSSRDF/paper.pdf
    //

    class NormalizedDiffusionBSSRDF
      : public BSSRDF
    {
      public:
        NormalizedDiffusionBSSRDF(
            const char*             name,
            const ParamArray&       params)
          : BSSRDF(name, params)
        {
            m_inputs.declare("reflectance", InputFormatSpectralReflectance);
            m_inputs.declare("mean_free_path", InputFormatSpectralReflectance);
            m_inputs.declare("mean_free_path_multiplier", InputFormatScalar, "1.0");
            m_inputs.declare("from_ior", InputFormatScalar);
            m_inputs.declare("to_ior", InputFormatScalar);
        }

        virtual void release() APPLESEED_OVERRIDE
        {
            delete this;
        }

        virtual const char* get_model() const APPLESEED_OVERRIDE
        {
            return Model;
        }

        virtual size_t compute_input_data_size(
            const Assembly&         assembly) const
        {
            return align(sizeof(NormalizedDiffusionBSSRDFInputValues), 16);
        }

        virtual void evaluate_inputs(
            const ShadingContext&   shading_context,
            InputEvaluator&         input_evaluator,
            const ShadingPoint&     shading_point,
            const size_t            offset = 0) const APPLESEED_OVERRIDE
        {
            BSSRDF::evaluate_inputs(shading_context, input_evaluator, shading_point, offset);

            char* ptr = reinterpret_cast<char*>(input_evaluator.data());
            NormalizedDiffusionBSSRDFInputValues* values =
                reinterpret_cast<NormalizedDiffusionBSSRDFInputValues*>(ptr + offset);

            values->m_mean_free_path *= static_cast<float>(values->m_mean_free_path_multiplier);

            if (values->m_mean_free_path.size() != values->m_reflectance.size())
            {
                if (values->m_mean_free_path.is_spectral())
                    Spectrum::upgrade(values->m_reflectance, values->m_reflectance);
                else
                    values->m_reflectance =
                        values->m_reflectance.convert_to_rgb(get_lighting_conditions());
            }
        }

        virtual void evaluate(
            const void*             data,
            const ShadingPoint&     outgoing_point,
            const Vector3d&         outgoing_dir,
            const ShadingPoint&     incoming_point,
            const Vector3d&         incoming_dir,
            Spectrum&               value) const APPLESEED_OVERRIDE
        {
            const NormalizedDiffusionBSSRDFInputValues* values =
                reinterpret_cast<const NormalizedDiffusionBSSRDFInputValues*>(data);

            const double dist = norm(incoming_point.get_point() - outgoing_point.get_point());

            value.resize(values->m_reflectance.size());
            for (size_t i = 0, e = value.size(); i < e; ++i)
            {
                const double a = values->m_reflectance[i];
                const double s = normalized_diffusion_s(a);
                const double ld = values->m_mean_free_path[i];
                value[i] = static_cast<float>(normalized_diffusion_r(dist, ld, s, a));
            }
        }

      private:
        virtual bool do_sample(
            const void*             data,
            BSSRDFSample&           sample,
            Vector2d&               point) const APPLESEED_OVERRIDE
        {
            const NormalizedDiffusionBSSRDFInputValues* values =
                reinterpret_cast<const NormalizedDiffusionBSSRDFInputValues*>(data);

            sample.set_is_directional(false);
            sample.set_eta(values->m_to_ior / values->m_from_ior);

            sample.get_sampling_context().split_in_place(3, 1);
            const Vector3d s = sample.get_sampling_context().next_vector2<3>();

            // Sample a color channel uniformly.
            const size_t channel = truncate<size_t>(s[0] * values->m_reflectance.size());
            sample.set_channel(channel);

            // Sample a radius and an angle.
            double radius =
                normalized_diffusion_sample(
                    normalized_diffusion_s(values->m_reflectance[channel]),
                    values->m_mean_free_path[channel],
                    s[1]);

            const double phi = TwoPi * s[2];
            point = Vector2d(radius * cos(phi), radius * sin(phi));
            return true;
        }

        virtual double do_pdf(
            const void*             data,
            const size_t            channel,
            const double            dist) const APPLESEED_OVERRIDE
        {
            const NormalizedDiffusionBSSRDFInputValues* values =
                reinterpret_cast<const NormalizedDiffusionBSSRDFInputValues*>(data);

            return normalized_diffusion_pdf(
                dist,
                normalized_diffusion_s(values->m_reflectance[channel]),
                values->m_mean_free_path[channel]);
        }
    };
}


//
// NormalizedDiffusionBSSRDFFactory class implementation.
//

const char* NormalizedDiffusionBSSRDFFactory::get_model() const
{
    return Model;
}

Dictionary NormalizedDiffusionBSSRDFFactory::get_model_metadata() const
{
    return
        Dictionary()
            .insert("name", Model)
            .insert("label", "Normalized Diffusion BSSRDF");
}

DictionaryArray NormalizedDiffusionBSSRDFFactory::get_input_metadata() const
{
    DictionaryArray metadata;

    metadata.push_back(
        Dictionary()
            .insert("name", "reflectance")
            .insert("label", "Reflectance")
            .insert("type", "colormap")
            .insert("entity_types",
                Dictionary()
                    .insert("color", "Colors")
                    .insert("texture_instance", "Textures"))
            .insert("use", "required")
            .insert("default", "0.5"));

    metadata.push_back(
        Dictionary()
            .insert("name", "mean_free_path")
            .insert("label", "Mean Free Path")
            .insert("type", "colormap")
            .insert("entity_types",
                Dictionary()
                    .insert("color", "Colors")
                    .insert("texture_instance", "Textures"))
            .insert("use", "required")
            .insert("default", "0.5"));

    metadata.push_back(
        Dictionary()
            .insert("name", "mean_free_path_multiplier")
            .insert("label", "Mean Free Path Multiplier")
            .insert("type", "colormap")
            .insert("entity_types",
                Dictionary().insert("texture_instance", "Textures"))
            .insert("use", "optional")
            .insert("default", "1.0"));

    metadata.push_back(
        Dictionary()
            .insert("name", "from_ior")
            .insert("label", "From Index of Refraction")
            .insert("type", "numeric")
            .insert("min_value", "0.0")
            .insert("max_value", "5.0")
            .insert("use", "required")
            .insert("default", "1.0"));

    metadata.push_back(
        Dictionary()
            .insert("name", "to_ior")
            .insert("label", "To Index of Refraction")
            .insert("type", "numeric")
            .insert("min_value", "0.0")
            .insert("max_value", "5.0")
            .insert("use", "required")
            .insert("default", "1.3"));

    return metadata;
}

auto_release_ptr<BSSRDF> NormalizedDiffusionBSSRDFFactory::create(
    const char*         name,
    const ParamArray&   params) const
{
    return auto_release_ptr<BSSRDF>(new NormalizedDiffusionBSSRDF(name, params));
}

}   // namespace renderer
