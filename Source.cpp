#include "../SDK/PluginSDK.h"
#include "../SDK/EventArgs.h"
#include "../SDK/EventHandler.h"

PLUGIN_API const char PLUGIN_PRINT_NAME[32] = "Jax";
PLUGIN_API const char PLUGIN_PRINT_AUTHOR[32] = "BiggieSmalls";
PLUGIN_API ChampionId PLUGIN_TARGET_CHAMP = ChampionId::Jax;

namespace Menu
{
	IMenu* MenuInstance = nullptr;
	IMenuElement* Toggle = nullptr;

	namespace Q
	{
		IMenuElement* InCombo = nullptr;
		IMenuElement* InHarass = nullptr;
		//IMenuElement* InLaneClear = nullptr;
		IMenuElement* InJungleClear = nullptr;
		IMenuElement* InFlee = nullptr;
	}

	namespace W
	{
		IMenuElement* InCombo = nullptr;
		IMenuElement* InHarass = nullptr;
		IMenuElement* InLaneClear = nullptr;
		IMenuElement* InJungleClear = nullptr;
	}

	namespace E
	{
		IMenuElement* UseE = nullptr;
		IMenuElement* Stun = nullptr;
		IMenuElement* Interruptables = nullptr;
		IMenuElement* EDanger = nullptr;
		IMenuElement* ElowHP = nullptr;
	}

	namespace R
	{
		IMenuElement* UseR = nullptr;
		IMenuElement* Rhp = nullptr;
		IMenuElement* Rmin = nullptr;
	}

	namespace Visuals
	{
		IMenuElement* Toggle = nullptr;
		IMenuElement* DrawQRange = nullptr;
		IMenuElement* DrawWRange = nullptr;
		IMenuElement* DrawERange = nullptr;
		IMenuElement* DrawRRange = nullptr;
	}

	namespace Colors
	{
		IMenuElement* SpellRanges = nullptr;
		IMenuElement* CooldownRanges = nullptr;
	}
}

namespace Spells
{
	std::shared_ptr<ISpell> Q = nullptr;
	std::shared_ptr<ISpell> W = nullptr;
	std::shared_ptr<ISpell> E = nullptr;
	std::shared_ptr<ISpell> R = nullptr;
}

//ward jump minions
bool IsMinionInRange()
{
	auto Minions = g_ObjectManager->GetMinionsAll();
	for (auto& Minion : Minions)
	{
		if (Minion->IsInRange(Spells::Q->Range()))
			return true;
	}
	return false;
}
//number of  enemies in Ult Radius
int CountEnemiesInRange(const Vector& Position, const float Range)
{
	auto Enemies = g_ObjectManager->GetChampions(false);
	int Counter = 0;
	for (auto& Enemy : Enemies)
		if (Enemy->IsVisible() && Enemy->IsValidTarget() && Position.Distance(Enemy->Position()) <= Range)
			Counter++;
	return Counter;
}

bool CompareDistanceToCursor(IGameObject* a, IGameObject* b)
{
	return a->Distance(g_Common->CursorPosition()) < b->Distance(g_Common->CursorPosition());
}


IGameObject* MouseUnit()
{
	// need to add Ally Champions too
	auto minions = g_ObjectManager->GetByType(EntityType::AIMinionClient);
	std::sort(minions.begin(), minions.end(), [&](IGameObject* a, IGameObject* b) { return CompareDistanceToCursor(a, b); });

	const auto unit = minions.front();
	return unit;
}

void OnAfterAttack(IGameObject* target)
{
	// AA reset
	if (Spells::W->IsReady())
	{
		const auto OrbwalkerTarget = g_Orbwalker->GetTarget();
		if (OrbwalkerTarget && OrbwalkerTarget->IsInRange(Spells::W->Range()) && OrbwalkerTarget->IsValidTarget())
		{
			// w in combo
			if (Menu::W::InCombo->GetBool() && g_Orbwalker->IsModeActive(eOrbwalkingMode::kModeCombo))
			{
				Spells::W->Cast();
				g_Orbwalker->ResetAA();
			}
		}
		{
			// w in harass
			if (Menu::W::InHarass->GetBool() && g_Orbwalker->IsModeActive(eOrbwalkingMode::kModeHarass))
			{
				Spells::W->Cast();
				g_Orbwalker->ResetAA();
			}
		}
		// w jungle clear
		{
			if (target->IsMonster() && Menu::W::InJungleClear->GetBool() && g_Orbwalker->IsModeActive(eOrbwalkingMode::kModeLaneClear))

			{
				Spells::W->Cast();
				g_Orbwalker->ResetAA();
			}
		}
	}
}

void OnGameUpdate()
{
	if (!Menu::Toggle->GetBool() || g_LocalPlayer->IsDead())
		return;

	if (Spells::Q->IsReady())
	{
		// Q logic for Combo and Harass
		const auto Enemy = g_Common->GetTargetFromCoreTS(Spells::Q->Range(), DamageType::Physical);

		{
			if (Enemy && Enemy->IsValidTarget())
			if (Menu::Q::InCombo->GetBool() && Enemy->IsInRange(Spells::Q->Range()) && g_Orbwalker->IsModeActive(eOrbwalkingMode::kModeCombo))

				if (Enemy && Enemy->IsValidTarget() && g_LocalPlayer->Distance(Enemy) <= 680)
					Spells::Q->Cast(Enemy);

			if (Menu::Q::InHarass->GetBool() && Enemy->IsInRange(Spells::Q->Range()) && g_Orbwalker->IsModeActive(eOrbwalkingMode::kModeHarass))

				if (Enemy && Enemy->IsValidTarget() && g_LocalPlayer->Distance(Enemy) <= 680)
					Spells::Q->Cast(Enemy);
		}

		//Q logic for Flee  ## need to improve direction ## ## add ward jump ##
		{
			if (Menu::Q::InFlee->GetBool() && g_Orbwalker->IsModeActive(eOrbwalkingMode::kModeFlee) && MouseUnit()->IsInRange(Spells::Q->Range()) && g_LocalPlayer->Distance(MouseUnit()) >= 300)
				Spells::Q->Cast(MouseUnit());
		}
		{
			// Q Jungle Clear
			auto Monsters = g_ObjectManager->GetJungleMobs();
			for (auto Monster : Monsters)

				if (Monster && Menu::Q::InJungleClear->GetBool() && Monster->IsInRange(Spells::Q->Range() && g_Orbwalker->IsModeActive(eOrbwalkingMode::kModeLaneClear)))
					Spells::Q->Cast(Monster);
		}
	}

	{
		// W Lane Clear Last Hit
		const auto Enemies = g_ObjectManager->GetMinionsEnemy();
		for (auto Enemy : Enemies)

			if (Enemy && Menu::W::InLaneClear->GetBool() && Enemy->IsInRange(Spells::W->Range()) && Spells::W->IsReady() && (g_Orbwalker->IsModeActive(eOrbwalkingMode::kModeLaneClear) || g_Orbwalker->IsModeActive(eOrbwalkingMode::kModeHarass)))
			{
				auto WDamage = g_Common->GetSpellDamage(g_LocalPlayer, Enemy, SpellSlot::W, false) + g_LocalPlayer->AutoAttackDamage(Enemy, true);
				if (Enemy->IsMinion() && WDamage >= Enemy->RealHealth(true, false))
					Spells::W->Cast();
			}
	}
	{
		// R logic
		if (Spells::R->IsReady() && Menu::R::UseR->GetBool())
		{
			if (CountEnemiesInRange(g_LocalPlayer->Position(), 400.f) >= Menu::R::Rmin->GetInt())
				Spells::R->Cast();
		}
		{
			if (CountEnemiesInRange(g_LocalPlayer->Position(), 370.f) && g_LocalPlayer->HealthPercent() <= Menu::R::Rhp->GetInt() && g_Orbwalker->IsModeActive(eOrbwalkingMode::kModeCombo))
				Spells::R->Cast();
		}
	}


	// E stun logic
	{
		if (Spells::E->IsReady() && Menu::E::UseE->GetBool())
		{
			if (CountEnemiesInRange(g_LocalPlayer->Position(), 350.f) >= Menu::E::Stun->GetInt() && g_LocalPlayer->HasBuff("JaxCounterStrike"))

				Spells::E->Cast();
		}

	}

	// Ward Placement for Q
	{
		if (Spells::Q->IsReady() && g_LocalPlayer->CanUseItem(ItemId::Warding_Totem_Trinket) && g_Orbwalker->IsModeActive(eOrbwalkingMode::kModeFlee))
			if (!IsMinionInRange())
				g_LocalPlayer->CastItem(ItemId::Warding_Totem_Trinket, g_LocalPlayer->Position().Extend(g_Common->CursorPosition(), 650.f));
	}
}

void OnProcessSpell(IGameObject* Owner, OnProcessSpellEventArgs* Args)
{

	if (Menu::E::UseE->GetBool() && Spells::E->IsReady() && g_LocalPlayer->Distance(Owner) < 350.f)
	{
		if (Owner->ChampionId() == ChampionId::Darius && Args->SpellSlot == SpellSlot::W)
			Spells::E->Cast();

		if (Owner->ChampionId() == ChampionId::Rammus && Args->SpellSlot == SpellSlot::E)
			Spells::E->Cast();

		if (Owner->ChampionId() == ChampionId::Yorick && Args->SpellSlot == SpellSlot::Q)
			Spells::E->Cast();

		if (Owner->ChampionId() == ChampionId::Garen && Args->SpellSlot == SpellSlot::Q)
			Spells::E->Cast();

		if (Owner->ChampionId() == ChampionId::Kled && Args->SpellSlot == SpellSlot::W)
			Spells::E->Cast();

		if (Owner->ChampionId() == ChampionId::Chogath && Args->SpellSlot == SpellSlot::E)
			Spells::E->Cast();

		if (Owner->ChampionId() == ChampionId::Illaoi && Args->SpellSlot == SpellSlot::W)
			Spells::E->Cast();

		if (Owner->ChampionId() == ChampionId::Singed && Args->SpellSlot == SpellSlot::E)
			Spells::E->Cast();

		if (Owner->ChampionId() == ChampionId::Rengar && Args->SpellSlot == SpellSlot::Q)
			Spells::E->Cast();

		if (Owner->ChampionId() == ChampionId::Nasus && Args->SpellSlot == SpellSlot::Q)
			Spells::E->Cast();

		if (Owner->ChampionId() == ChampionId::Camille && Args->SpellSlot == SpellSlot::Q)
			Spells::E->Cast();

		if (Owner->ChampionId() == ChampionId::Volibear && Args->SpellSlot == SpellSlot::Q)
			Spells::E->Cast();

		if (Owner->ChampionId() == ChampionId::Fiora && Args->SpellSlot == SpellSlot::E)
			Spells::E->Cast();



		if (Owner->ChampionId() == ChampionId::Riven && Args->SpellSlot == SpellSlot::Q)
			Spells::E->Cast();

		if (Owner->ChampionId() == ChampionId::Jayce && Args->SpellSlot == SpellSlot::E)
			Spells::E->Cast();



		if (Owner->ChampionId() == ChampionId::Aatrox && Args->SpellSlot == SpellSlot::E)
			Spells::E->Cast();

		if (Owner->ChampionId() == ChampionId::RekSai && Args->SpellSlot == SpellSlot::Q)
			Spells::E->Cast();

		if (Owner->ChampionId() == ChampionId::MonkeyKing && Args->SpellSlot == SpellSlot::Q)
			Spells::E->Cast();

		if (Owner->ChampionId() == ChampionId::Khazix && Args->SpellSlot == SpellSlot::Q)
			Spells::E->Cast();

		if (Owner->ChampionId() == ChampionId::Hecarim && Args->SpellSlot == SpellSlot::E)
			Spells::E->Cast();

		if (Owner->ChampionId() == ChampionId::Udyr && Args->SpellSlot == SpellSlot::E)
			Spells::E->Cast();

		if (Owner->ChampionId() == ChampionId::Vi && Args->SpellSlot == SpellSlot::R)
			Spells::E->Cast();

		if (Owner->ChampionId() == ChampionId::Vi && Args->SpellSlot == SpellSlot::E)
			Spells::E->Cast();

		if (Owner->ChampionId() == ChampionId::Nidalee && Args->SpellSlot == SpellSlot::Q)
			Spells::E->Cast();

		if (Owner->ChampionId() == ChampionId::Nocturne && Args->SpellSlot == SpellSlot::E)
			Spells::E->Cast();

		if (Owner->ChampionId() == ChampionId::MasterYi && Args->SpellSlot == SpellSlot::Q)
			Spells::E->Cast();

		if (Owner->ChampionId() == ChampionId::Shaco && Args->SpellSlot == SpellSlot::E)
			Spells::E->Cast();

		if (Owner->ChampionId() == ChampionId::Evelynn && Args->SpellSlot == SpellSlot::E)
			Spells::E->Cast();

		if (Owner->ChampionId() == ChampionId::Qiyana && Args->SpellSlot == SpellSlot::R)
			Spells::E->Cast();

		if (Owner->ChampionId() == ChampionId::Kassadin && Args->SpellSlot == SpellSlot::W)
			Spells::E->Cast();

		if (Owner->ChampionId() == ChampionId::Fizz && Args->SpellSlot == SpellSlot::W)
			Spells::E->Cast();

		if (Owner->ChampionId() == ChampionId::Renekton && Args->SpellSlot == SpellSlot::W)
			Spells::E->Cast();

		if (Owner->ChampionId() == ChampionId::Trundle && Args->SpellSlot == SpellSlot::Q)
			Spells::E->Cast();

		if (Owner->ChampionId() == ChampionId::Blitzcrank && Args->SpellSlot == SpellSlot::E)
			Spells::E->Cast();

		if (Owner->ChampionId() == ChampionId::Gragas && Args->SpellSlot == SpellSlot::W)
			Spells::E->Cast();

		if (Owner->ChampionId() == ChampionId::Urgot && Args->SpellSlot == SpellSlot::W)
			Spells::E->Cast();

		if (Owner->ChampionId() == ChampionId::Urgot && Args->SpellSlot == SpellSlot::E)
			Spells::E->Cast();

		if (Owner->ChampionId() == ChampionId::DrMundo && Args->SpellSlot == SpellSlot::E)
			Spells::E->Cast();

		if (Owner->ChampionId() == ChampionId::Talon && Args->SpellSlot == SpellSlot::Q)
			Spells::E->Cast();

		if (Owner->ChampionId() == ChampionId::Shyvana && Args->SpellSlot == SpellSlot::Q)
			Spells::E->Cast();

		if (Owner->ChampionId() == ChampionId::Irelia && Args->SpellSlot == SpellSlot::Q)
			Spells::E->Cast();



		if (Owner->ChampionId() == ChampionId::Garen && Owner->HasBuff("GarenQ"))
			Spells::E->Cast();
	}
	{
		//longer not targeted dangerous
		if (Menu::E::UseE->GetBool() && Spells::E->IsReady() && g_LocalPlayer->Distance(Owner) < 700.f)
		{
			if (Owner->ChampionId() == ChampionId::Kayle && Args->SpellSlot == SpellSlot::E)
				Spells::E->Cast();

			if (Owner->ChampionId() == ChampionId::Kaisa && Args->SpellSlot == SpellSlot::Q)
				Spells::E->Cast();


			if (Owner->ChampionId() == ChampionId::Warwick && Args->SpellSlot == SpellSlot::R)
				Spells::E->Cast();

			if (Owner->ChampionId() == ChampionId::Caitlyn && Owner->HasBuff("caitlynheadshot"))
				Spells::E->Cast();

			if (Owner->ChampionId() == ChampionId::Lucian && Owner->HasBuff("LucianPassiveShot"))
				Spells::E->Cast();

			if (Owner->ChampionId() == ChampionId::Diana && Args->SpellSlot == SpellSlot::R)
				Spells::E->Cast();

			if (Owner->ChampionId() == ChampionId::Ashe && Args->SpellSlot == SpellSlot::Q)
				Spells::E->Cast();

			if (Owner->ChampionId() == ChampionId::Vayne && Args->SpellSlot == SpellSlot::E)
				Spells::E->Cast();

			if (Owner->ChampionId() == ChampionId::Yasuo && Args->SpellSlot == SpellSlot::R)
				Spells::E->Cast();

			if (Owner->ChampionId() == ChampionId::Akali && Args->SpellSlot == SpellSlot::R)
				Spells::E->Cast();
		}

	}
	// ranged dangerous spells
	{
		if (Menu::E::UseE->GetBool() && Menu::E::EDanger->GetBool() && Spells::E->IsReady() && Args->Target == g_LocalPlayer && g_LocalPlayer->Distance(Owner) < 700.f)
		{
			{
				if (g_LocalPlayer->HealthPercent() <= Menu::E::ElowHP->GetInt() && Args->IsAutoAttack && Args->Target == g_LocalPlayer)
					Spells::E->Cast();
			}
			if (Owner->ChampionId() == ChampionId::TwistedFate && Owner->HasBuff("GoldCardPreAttack"))
				Spells::E->Cast();

			if (Owner->ChampionId() == ChampionId::Ekko && Args->SpellSlot == SpellSlot::E)
				Spells::E->Cast();

			if (Owner->ChampionId() == ChampionId::Skarner && Args->SpellSlot == SpellSlot::R)
				Spells::E->Cast();

			if (Owner->ChampionId() == ChampionId::Galio && Args->SpellSlot == SpellSlot::E)
				Spells::E->Cast();
			//questionable
			if (Owner->ChampionId() == ChampionId::Viktor && Args->SpellSlot == SpellSlot::Q)
				Spells::E->Cast();
			//possibly to interrupt after
			if (Owner->ChampionId() == ChampionId::Katarina && Args->SpellSlot == SpellSlot::R)
				Spells::E->Cast();

			if (Owner->ChampionId() == ChampionId::Quinn && Args->SpellSlot == SpellSlot::E)
				Spells::E->Cast();

			if (Owner->ChampionId() == ChampionId::Teemo && Args->SpellSlot == SpellSlot::Q)
				Spells::E->Cast();

			if (Owner->ChampionId() == ChampionId::MasterYi && Owner->HasBuff("MasterYiDoubleStrike"))
				Spells::E->Cast();

			if (Owner->ChampionId() == ChampionId::Twitch && Args->SpellSlot == SpellSlot::R)
				Spells::E->Cast();

			if (Owner->ChampionId() == ChampionId::KogMaw && Owner->HasBuff("KogMawBioArcaneBarrage"))
				Spells::E->Cast();

			if (Owner->ChampionId() == ChampionId::Jayce && Owner->HasBuff("JayceHyperCharge"))
				Spells::E->Cast();
		}
	}
}

void OnHudDraw()
{
	if (!Menu::Toggle->GetBool() || !Menu::Visuals::Toggle->GetBool() || g_LocalPlayer->IsDead())
		return;

	const auto PlayerPosition = g_LocalPlayer->Position();
	const auto CirclesWidth = 1.5f;

	if (Menu::Visuals::DrawQRange->GetBool())
		g_Drawing->AddCircle(PlayerPosition, Spells::Q->Range(), Spells::Q->IsReady() ? Menu::Colors::SpellRanges->GetColor() : Menu::Colors::CooldownRanges->GetColor(), CirclesWidth);

	if (Menu::Visuals::DrawWRange->GetBool())
		g_Drawing->AddCircle(PlayerPosition, Spells::W->Range(), Spells::W->IsReady() ? Menu::Colors::SpellRanges->GetColor() : Menu::Colors::CooldownRanges->GetColor(), CirclesWidth);

	if (Menu::Visuals::DrawERange->GetBool())
		g_Drawing->AddCircle(PlayerPosition, Spells::E->Range(), Spells::E->IsReady() ? Menu::Colors::SpellRanges->GetColor() : Menu::Colors::CooldownRanges->GetColor(), CirclesWidth);
}

PLUGIN_API bool OnLoadSDK(IPluginsSDK* plugin_sdk)
{
	DECLARE_GLOBALS(plugin_sdk);

	if (g_LocalPlayer->ChampionId() != ChampionId::Jax)
		return false;

	using namespace Menu;
	MenuInstance = g_Menu->CreateMenu("Jax","Jax by BiggieSmalls");
	Toggle = MenuInstance->AddCheckBox("Enabled", "global_toggle", true);

	const auto QSubMenu = MenuInstance->AddSubMenu("Q settings", "spell_q_menu");

	const auto QComboSubMenu = QSubMenu->AddSubMenu("Combo", "spell_q_menu_combo");
	Q::InCombo = QComboSubMenu->AddCheckBox("Enabled", "spell_q_menu_combo_enabled", false);

	const auto QHarassSubMenu = QSubMenu->AddSubMenu("Harass", "spell_q_menu_harass");
	Q::InHarass = QHarassSubMenu->AddCheckBox("Enabled", "spell_q_menu_harass_enabled", false);

	const auto QFleeSubMenu = QSubMenu->AddSubMenu("Flee", "spell_q_menu_flee");
	Q::InFlee = QFleeSubMenu->AddCheckBox("Enabled", "spell_q_menu_flee_enabled", true);

	const auto QFarmSubMenu = QSubMenu->AddSubMenu("Farm", "spell_q_menu_farm");
	//Q::InLaneClear = QFarmSubMenu->AddCheckBox("Use Q for lane clear", "spell_q_menu_farm_laneclear", false);
	Q::InJungleClear = QFarmSubMenu->AddCheckBox("Use Q for jungle clear", "spell_q_menu_farm_jngclear", false);

	const auto WSubMenu = MenuInstance->AddSubMenu("W settings", "spell_w_menu");

	const auto WComboSubMenu = WSubMenu->AddSubMenu("Combo", "spell_w_menu_combo");
	W::InCombo = WComboSubMenu->AddCheckBox("Enabled", "spell_w_menu_combo_enabled", true);

	const auto WHarassSubMenu = WSubMenu->AddSubMenu("Harass", "spell_w_menu_harass");
	W::InHarass = WHarassSubMenu->AddCheckBox("Enabled", "spell_w_menu_harass_enabled", true);

	const auto WFarmSubMenu = WSubMenu->AddSubMenu("Farm", "spell_w_menu_farm");
	W::InLaneClear = WFarmSubMenu->AddCheckBox("Use W for lane clear", "spell_w_menu_farm_laneclear", true);
	W::InJungleClear = WFarmSubMenu->AddCheckBox("Use W for jungle clear", "spell_w_menu_farm_jngclear", true);

	const auto ESubMenu = MenuInstance->AddSubMenu("E settings", "spell_e_menu");
	E::UseE = ESubMenu->AddCheckBox("Use E", "spell_e_combo_mode", true);
	E::Interruptables = ESubMenu->AddCheckBox("Interrupt Dangerous Channels", "spell_e_interrupt", true);
	E::EDanger = ESubMenu->AddCheckBox("On Dangerous Abilities", "spell_e_danger", true);
	E::ElowHP = ESubMenu->AddSlider("On Auto Attacks if HP  %", "spell_e_lowhp", 40, 0, 100);
	E::Stun = ESubMenu->AddSlider("Reactivate to Stun Min Enemy", "e__min_stun", 3, 0, 5);

	const auto RSubMenu = MenuInstance->AddSubMenu("R settings", "spell_r_menu");
	R::UseR = RSubMenu->AddCheckBox("Use R", "spell_R_use", true);
	R::Rhp = RSubMenu->AddSlider("Use in Combo at HP %", "combo_use_hp", 30, 0, 100);
	R::Rmin = RSubMenu->AddSlider("Auto use on Enemies in range ", "min_enemies_range", 2, 0, 5);

	const auto VisualsSubMenu = MenuInstance->AddSubMenu("Visuals", "visual_menu");
	Visuals::Toggle = VisualsSubMenu->AddCheckBox("Enable drawings", "visuals_toggle", true);
	Visuals::DrawQRange = VisualsSubMenu->AddCheckBox("Draw Q range", "visuals_q_range", false);
	Visuals::DrawWRange = VisualsSubMenu->AddCheckBox("Draw W range", "visuals_w_range", false);
	Visuals::DrawERange = VisualsSubMenu->AddCheckBox("Draw E range", "visuals_e_range", true);

	const auto ColorsSubMenu = MenuInstance->AddSubMenu("Colors", "color_menu");
	Colors::SpellRanges = ColorsSubMenu->AddColorPicker("Ready spells", "color_ready_spell", 0, 191, 255, 220);
	Colors::CooldownRanges = ColorsSubMenu->AddColorPicker("Cooldown spells", "color_cd_spell", 165, 165, 165, 165);

	Spells::Q = g_Common->AddSpell(SpellSlot::Q, 700.f);
	Spells::W = g_Common->AddSpell(SpellSlot::W, 240.f);
	Spells::E = g_Common->AddSpell(SpellSlot::E, 350.f);
	Spells::R = g_Common->AddSpell(SpellSlot::R);

	EventHandler<Events::OnProcessSpellCast>::AddEventHandler(OnProcessSpell);
	//EventHandler<Events::OnBuff>::AddEventHandler(OnBuffChange);
	EventHandler<Events::GameUpdate>::AddEventHandler(OnGameUpdate);
	EventHandler<Events::OnHudDraw>::AddEventHandler(OnHudDraw);
	EventHandler<Events::OnAfterAttackOrbwalker>::AddEventHandler(OnAfterAttack);

	g_Common->ChatPrint("<font color='#00BFFF'>Jax plugin was loaded, gl&hf!</font>");
	g_Common->Log("Jax plugin was loaded, gl&hf!");
	return true;
}

PLUGIN_API void OnUnloadSDK()
{
	EventHandler<Events::OnProcessSpellCast>::RemoveEventHandler(OnProcessSpell);
	//EventHandler<Events::OnBuff>::RemoveEventHandler(OnBuffChange);
	EventHandler<Events::GameUpdate>::RemoveEventHandler(OnGameUpdate);
	EventHandler<Events::OnHudDraw>::RemoveEventHandler(OnHudDraw);
	EventHandler<Events::OnAfterAttackOrbwalker>::RemoveEventHandler(OnAfterAttack);

	Menu::MenuInstance->Remove();

	g_Common->Log("Jax plugin was unloaded.");
}