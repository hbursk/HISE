/*  ===========================================================================
*
*   This file is part of HISE.
*   Copyright 2016 Christoph Hart
*
*   HISE is free software: you can redistribute it and/or modify
*   it under the terms of the GNU General Public License as published by
*   the Free Software Foundation, either version 3 of the License, or
*   (at your option) any later version.
*
*   HISE is distributed in the hope that it will be useful,
*   but WITHOUT ANY WARRANTY; without even the implied warranty of
*   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
*   GNU General Public License for more details.
*
*   You should have received a copy of the GNU General Public License
*   along with HISE.  If not, see <http://www.gnu.org/licenses/>.
*
*   Commercial licenses for using HISE in an closed source project are
*   available on request. Please visit the project's website to get more
*   information about commercial licensing:
*
*   http://www.hise.audio/
*
*   HISE is based on the JUCE library,
*   which must be separately licensed for closed source applications:
*
*   http://www.juce.com
*
*   ===========================================================================
*/

namespace hise { using namespace juce;


namespace
{
constexpr float stroke_width = 3.0;
}

CircularProgress::CircularProgress()
{
    setInterceptsMouseClicks( false, false );
}

void CircularProgress::paint( juce::Graphics& g )
{
    if ( m_progress < 0 || m_progress > 1.0 )
    {
        getLookAndFeel().drawSpinningWaitAnimation(
            g, Colours::white, 0, 0, getWidth(), getHeight() );
    }
    else
    {
        float radius =
            fmin( static_cast<float>( getWidth() / 2 ), static_cast<float>( getHeight() / 2 ) ) - stroke_width;
        juce::Path bg;
        float      cx = static_cast<float>( getWidth() / 2 );
        float      cy = static_cast<float>( getHeight() / 2 );

        const float two_pi = MathConstants<float>::twoPi;

        bg.addCentredArc( cx, cy, radius, radius, 0, 0, two_pi, true );

        juce::Path circleProgress;
        circleProgress.addCentredArc(
            cx, cy, radius, radius, 0, 0.0f, static_cast<float>( two_pi * m_progress.load() ), true );

        g.setColour( juce::Colours::darkgrey );
        g.strokePath( bg, juce::PathStrokeType( stroke_width ) );

        g.setColour( juce::Colours::white );
        g.strokePath( circleProgress, juce::PathStrokeType( stroke_width ) );

        g.setColour( juce::Colours::white );
        int  percent = static_cast<int>( m_progress.load() * 100 );
        auto pStr    = std::to_string( percent ) + "%";

        int textHeight = 20;
        int textWidth  = 40;
        g.setFont( Font(16.f) );
        g.drawText( pStr,
                    getWidth() / 2 - textWidth / 2 + 2,
                    getHeight() / 2 - textHeight / 2,
                    textWidth,
                    textHeight,
                    juce::Justification::centred );
    }

    startTimer( 1000 / 20 );
}

void CircularProgress::timerCallback()
{
    if ( isVisible() )
    {
        repaint();
    }
    else
    {
        stopTimer();
    }
}

void CircularProgress::progress( double p )
{
    m_progress.store( p );
}
}
