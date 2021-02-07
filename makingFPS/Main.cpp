#include <Siv3D.hpp>
#include <algorithm>
#include <array>
#include <cmath>
#include <numbers>
#include <optional>
#include <utility>
#include <vector>

class Eye {
 public:
  static constexpr double eye_range = std::numbers::pi / 2;
  static constexpr double eye_length = 300;
  static constexpr int eye_number = 2000;
  std::vector<std::pair<Line, double>> lines;
  const Vec2& pos_;
  const double& theta_;

  Eye(const Vec2& pos, const double& theta) : pos_(pos), theta_(theta) {
    update();
  }

  void update() {
    lines.clear();
    lines.reserve(eye_number);
    for (std::size_t i = 0; i < eye_number; ++i) {
      const double phi =
          (theta_ - eye_range / 2) + i * (eye_range / eye_number);
      lines.emplace_back(
          Line(pos_, eye_length * Vec2(std::cos(phi), std::sin(phi)) + pos_),
          std::abs(phi - theta_));
    }
  }

  void draw() {
    for (const auto& line : lines) {
      line.first.draw();
    }
  }
};

class Player {
 public:
  Vec2 pos;
  double theta = 0;
  double vel = 0.5;
  Eye eye;
  double turnvel = std::numbers::pi / 100;
  Player(Vec2 p) : pos(p.x, p.y), eye(pos, theta) {}
  void update() {
    if (KeyUp.pressed()) {
      pos.x += vel * std::cos(theta);
      pos.y += vel * std::sin(theta);
    }
    if (KeyDown.pressed()) {
      pos.x -= vel * std::cos(theta);
      pos.y -= vel * std::sin(theta);
    }
    if (KeyRight.pressed()) {
      theta += turnvel;
    }
    if (KeyLeft.pressed()) {
      theta -= turnvel;
    }

    eye.update();
  }
  void draw() {
    Circle(pos.x, pos.y, 30.0).draw(Color(0, 0, 255));
    eye.draw();
  }
};

class map {
 public:
  

};


std::vector<std::optional<Vec2>> makefocus(Player& Player,
                                           std::vector<Line>& walls) {
  std::vector<std::optional<Vec2>> focus;
  for (const auto& l : Player.eye.lines) {
    const auto& line = l.first;
    std::vector<Vec2> tmpfocus;
    for (const auto& wall : walls) {
      if (wall.intersectsAt(line).has_value()) {
        tmpfocus.push_back(wall.intersectsAt(line).value().asPoint());
        Circle(wall.intersectsAt(line).value().asPoint(), 5)
            .draw(Palette::Orange);
      }
    }
    if (tmpfocus.empty()) {
      focus.push_back(std::nullopt);
    } else {
      auto itr = std::min_element(tmpfocus.cbegin(), tmpfocus.cend(),
                                  [&Player](const auto& a, const auto& b) {
                                    return Geometry2D::Distance(a, Player.pos) <
                                           Geometry2D::Distance(b, Player.pos);
                                  });
      focus.push_back(*itr);
    }
  }
  return focus;
}

void drawFPSview(const std::vector<std::optional<Vec2>>& focus, const Player& Player) {
  for (int i = 0; i < Player.eye.lines.size(); i++) {
    const double window_width = Window::ClientSize().x;
    const double window_height = Window::ClientSize().y;
    const double tmp = ((window_width / 4) * 3 / Player.eye.lines.size()) * i;

    if (focus[i].has_value()) {
      const auto dist = Geometry2D::Distance(Player.pos, focus[i].value()) *
                        std::cos(Player.eye.lines[i].second);
      constexpr auto wall_height = 5000;
      Line(window_width / 4 + tmp, window_height / 2 - wall_height / dist,
           window_width / 4 + tmp, window_height / 2 + wall_height / dist)
          .draw();
    }
  }
}

void Main() {
  Window::SetStyle(WindowStyle::Sizable);
  Scene::SetScaleMode(ScaleMode::ResizeFill);
  Player Player({0, 0});
  std::vector<Line> walls;
  walls.emplace_back(10, 10, 100, 100);
  walls.emplace_back(100, 100, 50, 500);
  walls.emplace_back(50, 500, 10, 10);

  while (System::Update()) {
    Player.update();
    Player.draw();
    auto focus = makefocus(Player, walls);
    drawFPSview(focus, Player);
  }
}
