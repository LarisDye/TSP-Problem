"""Generate publication figures and TeX numbers from verified C++ outputs."""
import argparse
import csv
import json
import statistics
from pathlib import Path

import matplotlib
matplotlib.use('Agg')
import matplotlib.pyplot as plt
from matplotlib.patches import Polygon
import numpy as np
from verify_results import verify

ROOT = Path(__file__).resolve().parents[1]
BLUE, TEAL, ORANGE, INK = '#234e70', '#167d9a', '#c96c36', '#263b4a'


def save(fig, name):
    for extension in ('pdf', 'png'):
        fig.savefig(ROOT / 'figures' / f'{name}.{extension}', dpi=220, bbox_inches='tight', facecolor='white')
    plt.close(fig)


def map_base(ax, states):
    for f in states['features']:
        g = f['geometry']
        polygons = g['coordinates'] if g['type'] == 'MultiPolygon' else [g['coordinates']]
        for rings in polygons:
            ax.add_patch(Polygon(rings[0], closed=True, facecolor='#edf2f5', edgecolor='#b3c2cc', linewidth=.55))
    ax.set(xlim=(-126, -65), ylim=(24, 50.7), xlabel='Longitude', ylabel='Latitude')
    ax.set_aspect(1 / np.cos(np.deg2rad(38)))
    ax.spines[['top', 'right']].set_visible(False)
    ax.tick_params(colors='#687d8c', labelsize=8)


def main(directory):
    verify(directory)
    result = json.loads((directory / 'result.json').read_text())
    histories = list(csv.DictReader((directory / 'history.csv').open()))
    runs = list(csv.DictReader((directory / 'runs.csv').open()))
    # The surrounding TeX describes this exact, predeclared experiment. Reject
    # mixed/custom runs instead of silently producing figures with stale prose.
    if [int(r['seed']) for r in runs] != list(range(42, 62)) or result['seed'] != 42:
        raise ValueError('Paper figures require the 20-run experiment, seeds 42-61, with selected seed 42')
    for seed in range(42, 62):
        record = json.loads((directory / f'seed_{seed}' / 'result.json').read_text())
        if (record['cooling'] != .98 or record['min_ratio'] != 1e-4
                or record['steps_per_level'] != 4800 or record['max_levels'] != 2000):
            raise ValueError('Paper parameters differ from the saved experiment; regenerate the declared run')
    capitals = list(csv.DictReader((ROOT / 'data/att48_capitals.csv').open()))
    states = json.loads((ROOT / 'data/us48_states.geojson').read_text())
    xy = np.array([(float(c['longitude']), float(c['latitude'])) for c in capitals])
    points = np.array(result['coordinates'])
    route = np.array(result['route'] + result['route'][:1]) - 1
    plt.rcParams.update({'font.family': 'DejaVu Sans', 'font.size': 10,
                         'axes.labelcolor': INK, 'text.color': INK, 'axes.titleweight': 'bold',
                         'axes.titlesize': 13, 'pdf.fonttype': 42})
    # Same numbered vertices on the geographic map and exact benchmark plane.
    for with_route, name in [(False, 'att48_map'), (True, 'att48_solution')]:
        fig, ax = plt.subplots(figsize=(10.8, 5.7))
        map_base(ax, states)
        if with_route:
            ax.plot(xy[route, 0], xy[route, 1], color=TEAL, lw=1.4, zorder=3)
        ax.scatter(xy[:, 0], xy[:, 1], s=22, c=BLUE, edgecolors='white', linewidths=.5, zorder=4)
        # Northeast labels use leader lines and fixed, reproducible offsets.
        offsets = {6:(-23,-16),7:(20,-9),17:(7,9),18:(-30,-9),19:(24,2),27:(-26,7),
                   28:(-40,9),30:(-42,-3),36:(-29,0),37:(27,-10),43:(-20,15),44:(-22,-12)}
        for i, (x, y) in enumerate(xy, 1):
            offset = offsets.get(i, (5, 5))
            ax.annotate(str(i), (x,y), xytext=offset, textcoords='offset points', fontsize=7.6,
                        color=INK, zorder=5, bbox={'facecolor':'white','alpha':.85,'edgecolor':'none','pad':.2},
                        arrowprops={'arrowstyle':'-','color':'#8b9aa5','lw':.45} if i in offsets else None)
        if with_route:
            ax.scatter(*xy[0],s=95,facecolor=ORANGE,edgecolor='white',zorder=6,marker='*')
        title = f"ATT48 | SA tour: {result['best_length']:,} ATT units" if with_route else 'ATT48 | 48 contiguous US state capitals'
        ax.set_title(title, loc='left', pad=13)
        fig.text(.125,.018,'Natural Earth basemap and city locations. Geography is for display; optimization uses TSPLIB ATT costs.',fontsize=8,color='#687d8c')
        save(fig,name)

    fig, axes = plt.subplots(1,2,figsize=(10.8,4.2),sharex=True,sharey=True,layout='constrained')
    for ax, r, title in zip(axes,[np.array(result['initial_route']+result['initial_route'][:1])-1,route],
                             [f"Initial: {result['initial_length']:,}",f"Best: {result['best_length']:,}"]):
        ax.plot(points[r,0],points[r,1],color=TEAL,lw=.85,alpha=.85)
        ax.scatter(points[:,0],points[:,1],s=12,c=BLUE,zorder=3)
        ax.set_aspect('equal'); ax.set_title(title,loc='left')
        ax.set_xlabel('TSPLIB x'); ax.spines[['top','right']].set_visible(False)
    axes[0].set_ylabel('TSPLIB y')
    save(fig,'att48_coordinate_tours')

    x=np.array([int(h['iteration']) for h in histories])/1e6
    current=np.array([int(h['current']) for h in histories])
    best=np.array([int(h['best']) for h in histories])
    fig,(ax,zoom)=plt.subplots(1,2,figsize=(10.8,4.3),gridspec_kw={'width_ratios':[1.4,1]},layout='constrained')
    ax.plot(x,current,color='#a5b7c5',lw=.9,label='Current tour (end of level)')
    ax.step(x,best,where='post',color=TEAL,lw=1.8,label='Best so far (end of level)')
    ax.axhline(10628,color=ORANGE,ls='--',lw=1,label='Published optimum: 10,628')
    ax.set(title=f"Convergence | seed {result['seed']}",xlabel='Proposals (millions)',ylabel='ATT tour cost')
    ax.legend(frameon=False,fontsize=8,loc='upper right')
    zoom.step(x,best,where='post',color=TEAL,lw=1.8)
    zoom.axhline(10628,color=ORANGE,ls='--',lw=1)
    first=result['best_iteration']/1e6
    zoom.plot(first,result['best_length'],'o',color=ORANGE,ms=5)
    zoom.annotate(f"First optimum\n{result['best_iteration']:,} proposals",(first,result['best_length']),
                  xytext=(first+.08,11070),fontsize=8,arrowprops={'arrowstyle':'->','color':ORANGE})
    zoom.set(xlim=(.5,x[-1]),ylim=(10580,11550),title='Late search detail',xlabel='Proposals (millions)')
    for a in (ax,zoom):
        a.grid(axis='y',alpha=.18); a.spines[['top','right']].set_visible(False)
    save(fig,'att48_convergence')

    costs=[int(r['best']) for r in runs]
    fig,ax=plt.subplots(figsize=(10.8,2.85),layout='constrained')
    ax.bar([int(r['seed']) for r in runs],[(c/10628-1)*100 for c in costs],color=TEAL,width=.72)
    ax.set(title='20 independent runs | seeds 42-61',xlabel='Random seed',ylabel='Gap to optimum (%)',xticks=[int(r['seed']) for r in runs])
    ax.spines[['top','right']].set_visible(False); ax.grid(axis='y',alpha=.15)
    save(fig,'att48_runs')

    numbers = {
        'AttBest':str(result['best_length']), 'AttInitial':str(result['initial_length']),
        'AttTemperature':f"{result['initial_temperature']:.6f}", 'AttProposals':f"{result['proposals']:,}".replace(',',r'\,'),
        'AttAccepted':f"{result['accepted']:,}".replace(',',r'\,'),
        'AttFirstBest':f"{result['best_iteration']:,}".replace(',',r'\,'),
        'AttMean':f'{statistics.mean(costs):.2f}', 'AttMedian':f'{statistics.median(costs):.2f}',
        'AttStd':f'{statistics.stdev(costs):.2f}', 'AttWorst':str(max(costs)),
        'AttHits':str(costs.count(10628)), 'AttMeanGap':f'{(statistics.mean(costs)/10628-1)*100:.3f}',
        'AttReduction':f"{(1-result['best_length']/result['initial_length'])*100:.2f}",
        'AttMeanTime':f"{statistics.mean(float(r['seconds']) for r in runs):.4f}",
    }
    (ROOT/'results/att48/numbers.tex').write_text('% Generated by scripts/make_figures.py\n'+
        '\n'.join('\\newcommand{\\'+key+'}{'+value+'}' for key,value in numbers.items())+'\n',encoding='utf-8')
    # Keep long routes in several lines to fit the paper width.
    closed=result['route']+result['route'][:1]
    lines=[' '.join(str(i) for i in closed[a:a+12]) for a in range(0,len(closed),12)]
    (ROOT/'results/att48/route.txt').write_text('\n'.join(lines)+'\n')
    print(json.dumps(numbers,indent=2))


if __name__=='__main__':
    parser=argparse.ArgumentParser()
    parser.add_argument('--results',type=Path,default=ROOT/'results/att48')
    main(parser.parse_args().results)
