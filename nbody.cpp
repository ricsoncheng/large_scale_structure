#include "helpers.hpp"
#include "structures.hpp"

//helper functions

void group_bodies(Rect rect, std::vector<Body> bodies, int n, 
                  std::array<Rect,4> rects, std::array<std::vector<Body>,4> groups, std::array<int,4> ns){
        //init rects
        Real minx = rect.pos0.x;
        Real miny = rect.pos0.y;
        Real maxx = rect.pos1.x;
        Real maxy = rect.pos1.y;
        Real midx = (minx+maxx)/2;
        Real midy = (miny+maxy)/2;
        
        rects[0] = {rect.pos0,{midx,midy}};
        rects[1] = {{midx,miny},{maxx,midy}};
        rects[2] = {{minx,midy},{midx,maxy}};
        rects[3] = {{midx,midy},rect.pos1};
        //recursively call
        for(int i = 0; i < n; i++){
                for(int j = 0; j < 4; j++){
                        if(rects[j].contains(bodies[i].p)){
                                groups[j][ns[j]] = bodies[i];
                                ns[j] ++;
                                break;
                        }
                }
        }
}

Tree built_qtree_r(Rect rect, std::vector<Body> bodies, int n){
        std::vector<Tree> tree_children;
        Node node = {.rect = rect};
        node.body = NULL;
        Tree tree = {.node = node, .trees = tree_children};
        std::array<Rect> rects (4);
        std::array<std::vector<Body>,4> groups;
        std::array<int,4> ns;
        group_bodies(rect, bodies, n, rects, groups, ns);
        //recursively call
        int num_child = 4;
        for(int j = 0; j < 4; j++){
                if(ns[j] > 1){
                        tree.trees[j] = build_qtree_r(rects[j], groups[j], ns[j]);
                } else if (ns[j]){
                        Body body = groups[j][0];
                        tree.trees[j] = {{rects[j], {body.p, 0, body.mass}, body},NULL};
                } else {
                        num_child --;
                }
        }
        //compute center
        tree.num_child = num_child;
        std::vector<Rvec> centers (num_child);
        for(int j = 0; j < num_child; j++){
                centers[j] = tree.trees[j].center;
        }
        tree.center = mix_rvecs(centers, num_child);
        //return
        return tree;
}

void NBody::metric_expansion(Real dt){
        Real ratio = 1+hubble*dt;
        uargs.size *= ratio;
        for(int i = 0; i < num_bodies; i++){
                bodies[i].p = bodies[i].p.M(ratio);
        }
}

void NBody::border_wrap(){
        for(int i = 0; i < num_bodies; i++){
                bodies[i].p.x = fmod(bodies[i].p.x, uargs.size);
                bodies[i].p.y = fmod(bodies[i].p.y, uargs.size);
        }
}

void NBody::build_qtree(){
        return build_qtree_r({{0,0},{size,size}}, bodies, uargs.numbodies);
}

void NBody::leapfrog(Real dt){
        std::vector<Vec> accs = accelerations();
        for(int i = 0; i < num_bodies; i++){
                bodies[i].p.P(bodies[i].v.M(dt)); 
                bodies[i].v.P(accs[i].M(dt));
        }
}

Vec NBody::p_accel(Vec p1, Vec p2, Real mass, Real distance){
        Real mag_accel = uargs.gconst*mass/(distance*distance+plummer*plummer);
        Real dir_accel = atan2(p2.y-p1.y, p2.x-p1.x);
        return {acc*cos(dir_accel), acc*sin(dir_accel)};
}

Vec NBody::grid_accel(Vec pos){
        Vec accel = {0,0};
        int lat = uargs.lattice;
        //find all csom and distances
        std::vector<Vec> CsOM (lat*lat);
        std::vector<Real> distances (lat*lat);
        for(int x = -lat; i <= lat; i++){
                for(y = -lat; y <= lat; y++){
                        int index = x*lat+y;
                        Vec Com = {x*size, y*size};
                        CsOM[index] = COM;
                        distances[index] = distance(pos, COM);
                }
        }
        //compute acceleration
        for(int i = 0; i < lat*lat; i++){
                if(distances[i]*lattice < size){
                        accel.P(p_accel(pos, CsOM[i], 1, distances[i]));
                }
        }
        return accel;
}

<std::vector<std::vector<Vec>> NBody::GP_field(int mesh, REAL size){
        Real spacing = size/mesh;
        <std::vector<std::vector<Vec>> field (mesh, mesh);
        for(int i = 0; i < lattice; i++){
                for(int j = 0; j < lattice; j++){
                        Real px = (i+0.5)*spacing;
                        Real py = (j+0.5)*spacing;
                        field[i][j] = grid_accel({px,py});
                }
        }
        return field;
}

void NBody::init_field(){
        field = GP_field(uargs.mesh, uargs.size);
}

Vec NBody::pbc_accel(Vec pos, RVec center, Real lattice_dist){
        //rewrite with mesh
        Vec accel = {0,0};
        //if close
                //call p_accel
        //else
                //call grid_accel
}

Vec NBody::body_accel(int id){
        //don't use id
        Vec accel = {0,0};
        //rewrite with std::stack instead of tree_stack
        static Tree_stack BFSS = {0, Tree[uargs.nbody]};
        BFSS.add(quadtree);
        while(BFSS.n){
                tree = BFSS.pop();
                Real lattice_dist = lat_dist(bodies[i].p, tree.center.var, uargs.size);
                if(tree.node.body != NULL && tree.node.body->id == id){
                        continue;
                } else if (tree.node.center.var == 0 || 
                           lattice_dist/sqrt(tree.center.var) > QTR){
                        accel.P(lattice_accel(bodies[i].p, tree.node.center, lattice_dist));
                } else {
                        for(int i = 0; i < tree.num_child; i++){
                                BFSS.add(tree.trees[i]);
                        }
                }
        }
        return accel;
}

std::vector<Vec> NBody::accelerations(){ 
        static std::vector<Vec> accs (this.bodies.size());
        for(std::size_t i = 0; i < this.bodies.size(); i++){
                accs[i] = body_accel(i);
        }
        return accs;
}

void NBody::step(){
        build_qtree();
        leapfrog();
        border_wrap();
        metric_expansion();
        if(this.dargs != NULL){
                draw(this.dargs);
        }
}

void NBody::simulate(verbose = True){
        for(int i = 0; i < this.simtime/this.timestep; i++){
                if(verbose){
                        printf("%f\n", dt*i);
                }
                step(this.timestep, {simname+to_string(i)});
        }
}

void NBody::draw(string filename){
        Real dsize = this.dargs.size;
        static bool init = true;
        static std::vector<bool> pixelarr;
        if(init){
                pixelarr = new bool[dsize][dsize];
                init = false;
        }
        memset(parr, false, dsize*dsize*sizeof(bool));
        
        for(auto body: this.bodies){
                int x = body.p.x/uargs.size*dsize;
                int y = body.p.x/uargs.size*dsize;
                to_image[x%dsize][y%dsize] = true;
        }
        to_image(parr, dsize, filename);
}