from sys import exit

import directory2datasets

class Node:
    def __init__(self, id, data):
        self.id = id
        self.module = data['module']
        self.parent_id = data['parent']  
        self.children = []
        self.data = data
            
class Graph:
    def __init__(self):
        self.root = None
        self.linked = False
        self.nodes = {}

    """
    Prints graph information
    """
    def __repr__(self):
        s = ''
        if self.linked:
            s += 'Status: linked\n'
        else:
            s += 'Status: unlinked\n'

        for node_id in self.nodes:
            node = self.nodes[node_id]
            s += '\n%s (%s):\n' % (node.id, node.module)

            s += '\tChildren - %s\n\tData - %s\n' % (node.children, node.data)

        return s

    """
    Prints graph diagram
    """
    def __str__(self):
        if self.linked:
            s = 'Pipeline Diagram:\n\n'
            s += self._print_node(self.root, [], True)
            return s
        else:
            print('warning: cannot print graph diagram because nodes have not been linked')
            return self.__repr__()


    """
    Helper method for recursively printing nodes in a tree
    """
    def _print_node(self, node, levels, is_last):
        s = ''

        # gaps and bridges
        for l in levels:
            s += '\t' + '│' * l

        # T or end
        if levels:
            if is_last:
                s += '└─ '
            else:
                s += '├─ '

        # print node
        s += '%s (%s)\n' % (node.id, node.module)

        # recurse for child nodes
        if node.children:
            if not is_last:
                levels[-1] = 1

            levels.append(0)

            n = len(node.children)-1
            for i,child_node in enumerate(node.children):
                s += self._print_node(child_node, levels[:], (i == n))

        return s
    
    """
    Add an unlinked node to the graph
    """
    def add_node(self, node_id, node_data):
        node = Node(node_id, node_data)

        if node_id == 'datasets':
            if self.root is not None:
                print('warning: multiple root nodes exist')
            self.root = node

        self.nodes[node_id] = node

    """
    Link nodes in graph by specifying the children
    """
    def link_nodes(self):
        if self.root == None:
            exit('error: cannot create linkage because there is no root node')

        for node_id in self.nodes:
            node = self.nodes[node_id]
            if node.parent_id is not None:
                self.nodes[node.parent_id].children.append(node)
        
        self.linked = True
        
    """
    Executes the pipeline for the defined root node datasets
    """
    def execute(self, dryrun=False, verbose=False):
        # generate datasets from root node
        datasets = directory2datasets.convert(self.root['path'], dryrun, verbose)
        
        # process images in new directories
        processed_dirs = graph_processor.process(unprocessed_dirs, configs, dryrun=args.dryrun, verbose=args.verbose)

        # update processed.json
        processed.update(processed_dirs)
        if not args.dryrun:
            with processed_path.open(mode='w') as f:
                f.write(json.dumps(processed, indent=4))

        if args.verbose:
            print('processed.json...')
            print(json.dumps(processed_json, indent=4))