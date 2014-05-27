/*
 * Software License Agreement (BSD License)
 *
 *  Copyright (c) 2014 Max-Planck-Institute for Intelligent Systems
 *    Manuel Wuthrich (manuel.wuthrich@gmail.com)
 *
 *  All rights reserved.
 *
 *  Redistribution and use in source and binary forms, with or without
 *  modification, are permitted provided that the following conditions
 *  are met:
 *
 *   * Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 *   * Redistributions in binary form must reproduce the above
 *     copyright notice, this list of conditions and the following
 *     disclaimer in the documentation and/or other materials provided
 *     with the distribution.
 *   * Neither the name of Willow Garage, Inc. nor the names of its
 *     contributors may be used to endorse or promote products derived
 *     from this software without specific prior written permission.
 *
 *  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 *  "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 *  LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
 *  FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
 *  COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
 *  INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
 *  BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 *  LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
 *  CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 *  LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN
 *  ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 *  POSSIBILITY OF SUCH DAMAGE.
 *
 */

/**
 * @author Manuel Wuthrich (manuel.wuthrich@gmail.com)
 * @author Jan Issac (jan.issac@gmail.com)
 * Max-Planck-Institute for Intelligent Systems
 */

#ifndef STATE_FILTERING_DISTRIBUTION_BROWNIAN_DAMPED_BROWNIAN_MOTION_HPP
#define STATE_FILTERING_DISTRIBUTION_BROWNIAN_DAMPED_BROWNIAN_MOTION_HPP

#include <Eigen/Dense>

#include <boost/assert.hpp>

#include <state_filtering/distribution/distribution.hpp>
#include <state_filtering/distribution/sampleable.hpp>
#include <state_filtering/distribution/gaussian_mappable.hpp>
#include <state_filtering/distribution/gaussian/gaussian_distribution.hpp>

namespace filter
{

template <typename Traits>
class DampedBrownianMotion:
        public Distribution<Traits>,
        public Sampleable<Traits>,
        public GaussianMappable<Traits>
{
public:
    typedef typename Traits::ScalarType ScalarType;
    typedef typename Traits::SampleType SampleType;
    typedef typename Traits::CovarianceType CovarianceType;

    virtual ~DampedBrownianMotion() { }

    virtual SampleType sample()
    {
        return SampleType();
    }

    virtual SampleType mapFromGaussian(const SampleType& sample) const
    {
        return distribution_.mapFromGaussian(sample);
    }

    virtual void conditionals(const double& delta_time,
                              const SampleType& velocity,
                              const SampleType& acceleration)
    {
        // TODO this hack is necessary at the moment. the gaussian distribution cannot deal with
        // covariance matrices which are not full rank, which is the case for time equal to zero
        double bounded_delta_time = delta_time;
        if(bounded_delta_time < 0.00001) bounded_delta_time = 0.00001;

        distribution_.mean(Expectation(velocity, acceleration, bounded_delta_time));
        distribution_.covariance(Covariance(bounded_delta_time));
    }

    virtual void parameters(const double& damping, const CovarianceType& acceleration_covariance)
    {
        damping_ = damping;
        acceleration_covariance_ = acceleration_covariance;
    }

private:
    SampleType Expectation(
            const SampleType& velocity,
            const SampleType& acceleration,
            const double& delta_time)
    {
        SampleType velocity_expectation =
                (1.0 - exp(-damping_*delta_time))
                / damping_ * acceleration + exp(-damping_*delta_time)*velocity;

        // if the damping_ is too small, the result might be nan, we thus return the limit for
        // damping_ -> 0
        if(!std::isfinite(velocity_expectation.norm()))
            velocity_expectation = velocity + delta_time * acceleration;

        return velocity_expectation;
    }

    CovarianceType Covariance(const double& delta_time)
    {
        double factor = (1.0 - exp(-2.0*damping_*delta_time))/(2.0*damping_);
        if(!std::isfinite(factor))
            factor = delta_time;

        return factor * acceleration_covariance_;
    }

private:
    // conditionals
    GaussianDistribution<Traits> distribution_;

    // parameters
    double damping_;
    CovarianceType acceleration_covariance_;

    /** @brief euler-mascheroni constant */
    static const double gamma_ = 0.57721566490153286060651209008240243104215933593992;
};

}

#endif