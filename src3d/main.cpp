#include "batch.h"
#include "output.h"
#include "jolt_runtime.h"
#include "ui/app.h"
#include "result_archive.h"
#include "cli_options.h"
#include <Windows.h>
#include <iostream>
#include <iomanip>
namespace {
rs3d::BatchControl* activeControl=nullptr;
std::mutex consoleMutex;
BOOL WINAPI interrupt(DWORD signal){std::lock_guard<std::mutex>lock(consoleMutex);if((signal==CTRL_C_EVENT||signal==CTRL_BREAK_EVENT||signal==CTRL_CLOSE_EVENT)&&activeControl){activeControl->cancel();return TRUE;}return FALSE;}
using rs3d::cli::integer;
int selfTest(const std::filesystem::path&folder){
    struct Case { const wchar_t* name; std::filesystem::path argument; };
    std::vector<Case> cases;
    for(auto name:{L"rs3d_cli_tests.exe",L"rs3d_abi.exe",L"rs3d_input_tests.exe",L"rs3d_geometry_tests.exe",L"rs3d_morphology_tests.exe",L"rs3d_terrain_tests.exe",L"rs3d_physics_tests.exe",L"rs3d_elastic_tests.exe",L"rs3d_statistics_tests.exe",L"rs3d_config_tests.exe",L"rs3d_settling_tests.exe",L"rs3d_batch_tests.exe",L"rs3d_session_tests.exe",L"rs3d_contact_observer_tests.exe",L"rs3d_obstacle_tests.exe",L"rs3d_analysis_tests.exe",L"rs3d_motion_tests.exe",L"rs3d_archive_tests.exe",L"rs3d_playback_tests.exe",L"rs3d_ui_tests.exe"})
        cases.push_back({name,{}});
    auto root=folder;std::filesystem::path preset;
    for(int i=0;i<6;++i){auto candidate=root/"configs3d"/"obstacles_heatmap_timeline.ini";if(std::filesystem::exists(candidate)){preset=candidate;break;}root=root.parent_path();}
    if(preset.empty())throw std::runtime_error("self-test requires configs3d/obstacles_heatmap_timeline.ini beside the application or in the project");
    cases.push_back({L"rs3d_elastic_tests.exe",preset});cases.push_back({L"rs3d_ui_tests.exe",preset});
    for(const auto&test:cases){
        auto path=folder/test.name;if(!std::filesystem::exists(path))path=folder/L"selftests"/test.name;
        std::wstring command=L"\""+path.wstring()+L"\"";
        if(!test.argument.empty())command+=L" \""+test.argument.wstring()+L"\"";
        STARTUPINFOW startup{};startup.cb=sizeof(startup);PROCESS_INFORMATION process{};
        if(!CreateProcessW(path.c_str(),command.data(),nullptr,nullptr,FALSE,0,nullptr,folder.c_str(),&startup,&process))
            throw std::runtime_error("self-test tool missing: build all targets or use the full release package");
        WaitForSingleObject(process.hProcess,INFINITE);DWORD code=1;GetExitCodeProcess(process.hProcess,&code);
        CloseHandle(process.hThread);CloseHandle(process.hProcess);if(code)return int(code);
    }
    std::cout<<"PASS all "<<cases.size()<<" self-test cases\n";return 0;
}
}
int wmain(int argc,wchar_t**argv){SetConsoleOutputCP(CP_UTF8);try{
    rs3d::gui::AppOptions app;bool headless=false,single=false,self=false,previewSet=false;std::optional<std::uint64_t>count,seed;int preview=0;std::filesystem::path configPath,outputPath,replayPath,writeDefault;std::uint64_t replayId=1;bool throughput=false;
    for(int i=1;i<argc;++i){std::wstring arg=argv[i];auto next=[&](){if(++i>=argc)throw std::runtime_error("missing option value");return std::wstring(argv[i]);};
        if(arg==L"--tour")app.tour=true;else if(arg==L"--demo")app.demo=true;else if(arg==L"--tab"){app.tab=std::filesystem::path(next()).string();if(app.tab!="scene"&&app.tab!="run"&&app.tab!="analysis")throw std::runtime_error("tab must be scene, run or analysis");}else if(arg==L"--procedural-terrain")app.terrainTex="procedural";else if(arg==L"--terrain-tex")app.terrainTex=std::filesystem::path(next()).u8string();else if(arg==L"--shots")app.shotsDir=next();else if(arg==L"--input-log")app.inputLog=next();else if(arg==L"--scale"){app.scale=float(rs3d::cli::real(next(),.5,3,"scale"));}
        else if(arg==L"--size"){auto size=rs3d::cli::windowSize(next());app.width=size.first;app.height=size.second;}
        else if(arg==L"--duration"){auto value=next();size_t used=0;app.autoExitSeconds=std::stod(value,&used);if(used!=value.size()||!std::isfinite(app.autoExitSeconds)||app.autoExitSeconds<0)throw std::runtime_error("duration must be finite and nonnegative");}
        else if(arg==L"--config")configPath=next();else if(arg==L"--output"){outputPath=next();}else if(arg==L"--headless")headless=true;else if(arg==L"--single")single=true;else if(arg==L"--batch")count=integer(next());else if(arg==L"--seed")seed=integer(next());else if(arg==L"--preview"){auto n=integer(next());if(n!=0&&n!=2)throw std::runtime_error("preview must be 0 or 2");preview=int(n);previewSet=true;}
        else if(arg==L"--view-results")app.viewResults=next();else if(arg==L"--diagnostic-results")app.diagnostic=true;
        else if(arg==L"--self-test")self=true;else if(arg==L"--replay")replayPath=next();else if(arg==L"--trial")replayId=integer(next());else if(arg==L"--write-config")writeDefault=next();else if(arg==L"--throughput-scene")throughput=true;
        else if(arg==L"--help"){std::cout<<"RollingStone3D: SI units, x downslope / y across / z up.\n"
            "--config scene.ini --batch N --seed integer --preview 0 --headless [--output path]\n"
            "--config scene.ini --single [--headless] [--output path]\n"
            "--config scene.ini --batch N --preview 2   (real GUI previews, then background)\n"
            "--view-results run_directory [--headless] [--diagnostic-results]  (read-only history)\n"
            "--replay run_directory --trial id --headless\n--self-test\n--write-config scene.ini [--throughput-scene]\n"
            "GUI: [--config scene.ini] [--view-results dir] [--tab scene|run|analysis] [--demo] [--scale 1.25] [--size 1366x768]\n"
            "     [--terrain-tex NAME | --procedural-terrain] (default: bundled assets/textures/terrain; env RS3D_TERRAIN_TEX)\n"
            "     [--tour --shots dir] (automated walkthrough with screenshots) [--duration seconds] [--single|--batch N start immediately]\n"
            "No arguments opens the 3D app.\n";return 0;
        }else throw std::runtime_error("unknown option; see --help");
    }
    if(self){wchar_t executable[32768];DWORD length=GetModuleFileNameW(nullptr,executable,32768);if(!length||length>=32768)throw std::runtime_error("cannot locate test binaries");return selfTest(std::filesystem::path(executable).parent_path());}
    rs3d::JoltRuntime runtime;auto draft=configPath.empty()?rs3d::SceneDraft{}:rs3d::loadScene(configPath);
    if(!app.viewResults.empty()&&(single||count||!replayPath.empty()))throw std::runtime_error("--view-results is read-only and cannot combine with run/replay options");
    if(headless&&!app.viewResults.empty()){auto before=rs3d::physicsUpdateCount();auto archive=rs3d::openResultArchive(app.viewResults,app.diagnostic);auto directory=archive->analysis->directory;std::uint64_t frames=0;for(const auto&t:directory->trials)frames+=t->frames;std::cout<<"readonly_result N="<<directory->trials.size()<<" frames="<<frames<<" legacy="<<directory->legacy<<" diagnostic="<<directory->diagnostic<<" physics_updates="<<rs3d::physicsUpdateCount()-before<<'\n';return 0;}
    if(throughput){draft.terrain.type=rs3d::TerrainType::flat;draft.terrain.width=20;draft.releaseGap=0;draft.properties[rs3d::restitutionValue]={0,0};draft.maxTime=10;draft.trajectoryStride=0;draft.stableFaceRelease=true;}
    if(count)draft.count=*count;if(seed)draft.masterSeed=*seed;if(!outputPath.empty())draft.outputRoot=outputPath;
    if(!writeDefault.empty()){rs3d::saveScene(writeDefault,draft);std::cout<<"Saved UTF-8 scene: "<<writeDefault.u8string()<<'\n';return 0;}
    if(single&&count)throw std::runtime_error("--single and --batch are mutually exclusive");
    if(!headless&&replayPath.empty()){
#ifdef RS3D_GUI
        app.autoStartSingle=single;app.autoStartBatch=count.has_value();if(previewSet)app.preview=preview;return rs3d::gui::runApplication(draft,app);
#else
        throw std::runtime_error("this build excludes GUI; use --headless or build_3d.cmd Release");
#endif
    }
    if(preview)throw std::runtime_error("headless cannot display previews; use --preview 0");
    if(!replayPath.empty()){
        auto replay=rs3d::loadReplay(replayPath,replayId);rs3d::Trial trial(replay.scene,replay.settings);while(trial.snapshot().reason==rs3d::StopReason::running)trial.step();auto actual=trial.snapshot();double error=rs3d::norm(actual.center-replay.expected.center);bool same=rs3d::matchesReplay(actual,replay);
        double barrierError=replay.hasBarrierState?rs3d::norm(actual.barrier.center-replay.expected.barrier.center):0;
        std::cout<<std::setprecision(17)<<"replay trial="<<replayId<<" reason="<<rs3d::stopReasonName(actual.reason)<<" position_error_m="<<error<<" barrier_position_error_m="<<barrierError<<" pass="<<same<<'\n';return same?0:1;
    }
    if(single)draft.count=1;auto config=rs3d::freezeConfig(draft);rs3d::BatchControl control;{std::lock_guard<std::mutex>lock(consoleMutex);activeControl=&control;}SetConsoleCtrlHandler(interrupt,TRUE);
    rs3d::BatchOptions options;options.observer=[](const rs3d::BatchEvent&e){if(e.kind==rs3d::BatchEventKind::trialFinished&&(e.trialId<=3||e.trialId%50==0))std::cout<<"processed="<<e.processed<<" complete="<<e.completed<<" failures="<<e.failures<<'\n';};
    auto result=rs3d::runBatch(config,control,options);SetConsoleCtrlHandler(interrupt,FALSE);{std::lock_guard<std::mutex>lock(consoleMutex);activeControl=nullptr;}
    std::cout<<std::setprecision(10)<<"status="<<rs3d::batchStatusName(result.status)<<" N="<<draft.count<<" C="<<result.statistics.completed()<<" failures="<<result.failures<<" timeouts="<<result.timeouts<<" unstarted="<<result.unstarted()<<" updates="<<result.updates<<" internal_steps="<<result.internalSteps<<"\ngeneration_s="<<result.timing.generation<<" physics_s="<<result.timing.physics<<" recording_s="<<result.timing.recording<<" statistics_s="<<result.timing.statistics<<" output_s="<<result.timing.output<<" wall_s="<<result.timing.wall<<"\noutput="<<result.directory.u8string()<<"\nerror="<<result.error<<'\n';
    return result.status==rs3d::BatchStatus::complete?0:(result.status==rs3d::BatchStatus::cancelled?2:1);
}catch(const std::exception&e){std::cerr<<"ERROR: "<<e.what()<<'\n';return 1;}}
