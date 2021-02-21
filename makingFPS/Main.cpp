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
	static constexpr double eye_range = std::numbers::pi / 3;
	static constexpr double eye_length = 75;
	static constexpr int eye_number = 300;
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
  std::vector<Line> map_;

	Player(Vec2 p, const std::vector<Line>& map) : pos(p.x, p.y), eye(pos, theta) ,map_(map) {}

	void update() {

		const auto tmp_pos = pos;
		if (KeyUp.pressed()) {
			pos.x += vel * std::cos(theta);
			pos.y += vel * std::sin(theta);
		}
		if (KeyDown.pressed()) {
			pos.x -= vel * std::cos(theta);
			pos.y -= vel * std::sin(theta);
		}
		if (std::any_of(map_.cbegin(), map_.cend(), [this](const auto& a){ return a.intersects(pos); })) pos = tmp_pos;

		if (KeyRight.pressed()) {
			theta += turnvel;
		}
		if (KeyLeft.pressed()) {
			theta -= turnvel;
		}

		eye.update();
	}
	void draw() {
		Circle(pos.x, pos.y, 5.0).draw(Color(0, 0, 255));
		eye.draw();
	}
};

std::vector<std::optional<std::pair<Vec2,double>>> makefocus(const Player& Player,
	 const std::vector<Line>& walls) {

	const Vec2 light = Player.pos;

	std::vector<std::optional<std::pair<Vec2,double>>> focus;
	for (const auto& l : Player.eye.lines) {
		const auto& line = l.first;
	  std::vector<std::pair<Vec2,double>> tmpfocus;
		for (const auto& wall : walls) {
			if (wall.intersectsAt(line).has_value()) {
				const auto pos = wall.intersectsAt(line).value().asPoint();
				tmpfocus.emplace_back(pos,
					0.05 + 1000.0 * std::abs(wall.vector().normalized().cross(
						(light - pos).normalized())) / std::pow(Geometry2D::Distance(pos, light), 2));
				Circle(wall.intersectsAt(line).value().asPoint(), 5)
					.draw(Palette::Orange);
			}
		}
		if (tmpfocus.empty()) {
			focus.push_back(std::nullopt);
		}
		else {
			auto itr = std::min_element(tmpfocus.cbegin(), tmpfocus.cend(),
				[&Player](const auto& a, const auto& b) {
					return	Geometry2D::Distance(a.first, Player.pos) <
							Geometry2D::Distance(b.first, Player.pos);
				});
			focus.push_back(*itr);
		}
	}
	return focus;
}

void drawFPSview(const std::vector<std::optional<std::pair<Vec2, double>>>& focus, const Player& Player) {
	for (int i = 0; i < Player.eye.lines.size(); i++) {
		const double window_width = Window::ClientSize().x;
		const double window_height = Window::ClientSize().y;
		const double tmp = ((window_width / 4) * 3 / Player.eye.lines.size()) * i;

		if (focus[i].has_value()) {
			const auto dist = Geometry2D::Distance(Player.pos, focus[i].value().first) *

				std::cos(Player.eye.lines[i].second);

			constexpr auto wall_height = 5000;
			int a = focus[i].value().first.y;
			int b = focus[i].value().first.x;
			if (a % 25 == 0 && b % 25 == 0) {
				Line(window_width / 4 + tmp, window_height / 2 - wall_height / dist,
					window_width / 4 + tmp, window_height / 2 + wall_height / dist)
					.draw(Palette::Orange, HSV(32, 82, focus[i].value().second));
			}
			else {
				Line(window_width / 4 + tmp, window_height / 2 - wall_height / dist,
					window_width / 4 + tmp, window_height / 2 + wall_height / dist)
					.draw(HSV(0, 0, focus[i].value().second));
			}
		}
	}
}

std::vector<Line> makemap() {
	std::vector<Line> walls;

	constexpr int height = 27;
	constexpr int width = 9;
	double cell_size = (Window::ClientSize().x/4)/width;
	std::array<std::array<int, width>, height> map;
	for (auto& m : map) m.fill(1);


	enum class direction {
		up, 
		down,
		right,
		left
	};

	int digg_pos_x = 1;
	int digg_pos_y = 1;
	map[digg_pos_y][digg_pos_x] = 0;
	bool end_flag = false;
	while(digg_pos_x==10&&digg_pos_y==10) {
		
		const auto dir = static_cast<direction>(Random(0, 3));

		switch (dir) {
		case direction::up:
			if (digg_pos_y - 2 > 0) {
				digg_pos_y -= 2;
				if (map[digg_pos_y][digg_pos_x] == 1) {
					map[digg_pos_y][digg_pos_x] = 0;
					map[digg_pos_y + 1][digg_pos_x] = 0;
				}
				else digg_pos_y += 2;
			}
			else end_flag = true;
			break;
		case direction::down:
			if (digg_pos_y + 2 < height) {
				digg_pos_y += 2;
				if (map[digg_pos_y][digg_pos_x] == 1) {
					map[digg_pos_y][digg_pos_x] = 0;
					map[digg_pos_y - 1][digg_pos_x] = 0;
				}
				else digg_pos_y -= 2;
			}
			else end_flag = true;
			break;
		case direction::right:
			if (digg_pos_x + 2 < width) {
				digg_pos_x += 2;
				if (map[digg_pos_y][digg_pos_x] == 1) {
					map[digg_pos_y][digg_pos_x] = 0;
					map[digg_pos_y][digg_pos_x - 1] = 0;
				}
				else digg_pos_x -= 2;
			}
			else end_flag = true;
			break;
		case direction::left:
			if (digg_pos_x - 2 > 0) {
				digg_pos_x -= 2;
				if (map[digg_pos_y][digg_pos_x] == 1) {
					map[digg_pos_y][digg_pos_x] = 0;
					map[digg_pos_y][digg_pos_x + 1] = 0;
				}
				else digg_pos_x += 2;
			}
			else end_flag = true;
			break;
		}
		int Random_x = Random(0, width - 1);
		int Random_y = Random(0, height - 1);

		if (map[Random_y][Random_x] == 0 && end_flag) {
			digg_pos_x = Random_x;
			digg_pos_y = Random_y;
		}
	}

	for (auto m = map.begin(); m != map.end(); m++) {
	  for (auto n = m->begin(); n != m->end(); n++) {
		  if (m == map.begin() || m == map.end() - 1 || n == m->begin() || n == m->end() - 1) *n = 1;
		}
	}



	for (auto i = 0; i < height; i++) {
		Vec2 p;
		p.y = i * cell_size;
		for (auto j = 0; j < width; j++) {
			p.x = j * cell_size;
			if (map[i][j] == 1) {
				walls.emplace_back(p.x, p.y, p.x + cell_size, p.y);
				walls.emplace_back(p.x, p.y, p.x, p.y + cell_size);
				walls.emplace_back(p.x + cell_size, p.y + cell_size, p.x + cell_size, p.y);
				walls.emplace_back(p.x + cell_size, p.y + cell_size, p.x, p.y + cell_size);
			}
		}
	}
	return walls;
}

void drawmap(const std::vector<Line>& walls) {
	for (const auto& wall : walls) wall.draw();
}

void Main() {
	Window::SetStyle(WindowStyle::Sizable);
	Scene::SetScaleMode(ScaleMode::ResizeFill);
	const auto map = makemap();
	Player Player({ 30, 30 }, map);

	while (System::Update()) {
		Player.update();
		Player.draw();
		drawmap(map);
		drawFPSview(makefocus(Player, map), Player);

	}
}
