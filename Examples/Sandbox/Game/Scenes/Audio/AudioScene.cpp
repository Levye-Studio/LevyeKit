#include "AudioScene.hpp"

#include <Levye/LevyeKit.hpp>
namespace Sandbox {
void AudioScene::Initialize() {
  clickSound = Levye::Audio::LoadSound("click.wav");

  music = Levye::Audio::LoadMusic("music.ogg");

  Levye::Audio::SetMusicVolume(music, 0.5f);

  Levye::Audio::PlayMusic(music);
}
void AudioScene::Update() {
  if (Levye::Input::IsPressed("PauseMusic")) {
    Levye::Audio::PauseMusic(music);
  }
  if (Levye::Input::IsPressed("ResumeMusic")) {
    Levye::Audio::ResumeMusic(music);
  }

  if (Levye::Input::IsPressed("TestSound")) {
    Levye::Audio::PlaySound(clickSound);
  }
}
}  // namespace Sandbox
